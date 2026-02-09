#include "core/platform/linux/WlrScreenCapturer.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "core/Logger.h"
#include "core/RawImage.h"
#include "wlr-screencopy-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

namespace {

// ── Wayland 相关内部数据结构 ──

struct OutputInfo {
    wl_output* output = nullptr;
    zxdg_output_v1* xdg = nullptr;
    std::string name;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    float scale = 1.0f;
    bool primary = false;
};

struct WlrContext {
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_shm* shm = nullptr;
    zwlr_screencopy_manager_v1* manager = nullptr;
    zxdg_output_manager_v1* xdgOutputManager = nullptr;
    std::vector<std::unique_ptr<OutputInfo>> outputs;
};

// ── SHM 共享内存辅助 ──

int createShmFile(size_t size) {
    for (int attempt = 0; attempt < 8; ++attempt) {
        std::string shmName = "/c3-shm-" + std::to_string(getpid()) + "-" +
                              std::to_string(rand());
        int fd = shm_open(shmName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(shmName.c_str());
            if (ftruncate(fd, static_cast<off_t>(size)) == 0) {
                return fd;
            }
            close(fd);
        }
    }
    return -1;
}

struct ShmBuffer {
    wl_buffer* buffer = nullptr;
    void* data = nullptr;
    size_t size = 0;
    int width = 0;
    int height = 0;
    int stride = 0;
    uint32_t format = 0;
};

bool createShmBuffer(wl_shm* shm, int width, int height, int stride,
                     uint32_t format, ShmBuffer& out) {
    size_t bufSize = static_cast<size_t>(stride) * static_cast<size_t>(height);
    int fd = createShmFile(bufSize);
    if (fd < 0) {
        Logger::error("wlr: failed to create shm file");
        return false;
    }

    void* data =
        mmap(nullptr, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        Logger::error("wlr: failed to mmap shm");
        close(fd);
        return false;
    }

    wl_shm_pool* pool = wl_shm_create_pool(shm, fd, static_cast<int>(bufSize));
    wl_buffer* buffer =
        wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
    wl_shm_pool_destroy(pool);
    close(fd);

    if (!buffer) {
        munmap(data, bufSize);
        Logger::error("wlr: failed to create wl_buffer");
        return false;
    }

    out.buffer = buffer;
    out.data = data;
    out.size = bufSize;
    out.width = width;
    out.height = height;
    out.stride = stride;
    out.format = format;
    return true;
}

// ── screencopy frame 回调 ──

struct FrameCapture {
    wl_shm* shm = nullptr;
    zwlr_screencopy_frame_v1* frame = nullptr;
    ShmBuffer buffer{};
    bool bufferInfoReceived = false;
    bool bufferDone = false;
    bool ready = false;
    bool failed = false;
    bool yInvert = false;
    uint32_t format = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
};

void frameBuffer(void* data, zwlr_screencopy_frame_v1*, uint32_t format,
                 uint32_t width, uint32_t height, uint32_t stride) {
    auto* capture = static_cast<FrameCapture*>(data);
    capture->format = format;
    capture->width = width;
    capture->height = height;
    capture->stride = stride;
    capture->bufferInfoReceived = true;
    if ((capture->bufferDone ||
         zwlr_screencopy_frame_v1_get_version(capture->frame) < 3) &&
        !capture->buffer.buffer) {
        if (createShmBuffer(capture->shm, static_cast<int>(width),
                            static_cast<int>(height), static_cast<int>(stride),
                            format, capture->buffer)) {
            zwlr_screencopy_frame_v1_copy(capture->frame,
                                          capture->buffer.buffer);
        }
    }
}

void frameFlags(void* data, zwlr_screencopy_frame_v1*, uint32_t flags) {
    auto* capture = static_cast<FrameCapture*>(data);
    capture->yInvert = (flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT) != 0;
}

void frameReady(void* data, zwlr_screencopy_frame_v1*, uint32_t, uint32_t,
                uint32_t) {
    auto* capture = static_cast<FrameCapture*>(data);
    capture->ready = true;
}

void frameFailed(void* data, zwlr_screencopy_frame_v1*) {
    auto* capture = static_cast<FrameCapture*>(data);
    capture->failed = true;
}

void frameDamage(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t,
                 uint32_t) {}

void frameLinuxDmabuf(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t,
                      uint32_t) {}

void frameBufferDone(void* data, zwlr_screencopy_frame_v1*) {
    auto* capture = static_cast<FrameCapture*>(data);
    capture->bufferDone = true;
    if (capture->bufferInfoReceived && !capture->buffer.buffer) {
        if (createShmBuffer(capture->shm, static_cast<int>(capture->width),
                            static_cast<int>(capture->height),
                            static_cast<int>(capture->stride), capture->format,
                            capture->buffer)) {
            zwlr_screencopy_frame_v1_copy(capture->frame,
                                          capture->buffer.buffer);
        }
    }
}

const zwlr_screencopy_frame_v1_listener kFrameListener = {
    frameBuffer, frameFlags,       frameReady,     frameFailed,
    frameDamage, frameLinuxDmabuf, frameBufferDone};

// ── wl_output 回调 ──

void outputGeometry(void* data, wl_output*, int32_t x, int32_t y, int32_t,
                    int32_t, int32_t, const char*, const char*, int32_t) {
    auto* info = static_cast<OutputInfo*>(data);
    info->x = x;
    info->y = y;
}

void outputMode(void* data, wl_output*, uint32_t flags, int32_t width,
                int32_t height, int32_t) {
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        auto* info = static_cast<OutputInfo*>(data);
        info->w = width;
        info->h = height;
    }
}

void outputDone(void* data, wl_output*) {
    auto* info = static_cast<OutputInfo*>(data);
    if (info->name.empty()) {
        info->name = "wl_output";
    }
}

void outputScale(void* data, wl_output*, int32_t factor) {
    auto* info = static_cast<OutputInfo*>(data);
    info->scale = static_cast<float>(factor);
}

void outputName(void* data, wl_output*, const char* name) {
    auto* info = static_cast<OutputInfo*>(data);
    if (name) {
        info->name = name;
    }
}

void outputDescription(void*, wl_output*, const char*) {}

const wl_output_listener kOutputListener = {outputGeometry, outputMode,
                                            outputDone,     outputScale,
                                            outputName,     outputDescription};

// ── xdg-output 回调（用于获取逻辑坐标和名称）──

void xdgOutputLogicalPosition(void* data, zxdg_output_v1*, int32_t x,
                              int32_t y) {
    auto* info = static_cast<OutputInfo*>(data);
    info->x = x;
    info->y = y;
}

void xdgOutputLogicalSize(void* data, zxdg_output_v1*, int32_t width,
                          int32_t height) {
    auto* info = static_cast<OutputInfo*>(data);
    info->w = width;
    info->h = height;
}

void xdgOutputDone(void*, zxdg_output_v1*) {}

void xdgOutputName(void* data, zxdg_output_v1*, const char* name) {
    auto* info = static_cast<OutputInfo*>(data);
    if (name) {
        info->name = name;
    }
}

void xdgOutputDescription(void*, zxdg_output_v1*, const char*) {}

const zxdg_output_v1_listener kXdgOutputListener = {
    xdgOutputLogicalPosition, xdgOutputLogicalSize, xdgOutputDone,
    xdgOutputName, xdgOutputDescription};

// ── registry 回调 ──

void registryGlobal(void* data, wl_registry* registry, uint32_t name,
                    const char* interface, uint32_t version) {
    auto* ctx = static_cast<WlrContext*>(data);
    if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        ctx->shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, wl_output_interface.name) == 0) {
        auto output = std::make_unique<OutputInfo>();
        output->output = static_cast<wl_output*>(wl_registry_bind(
            registry, name, &wl_output_interface, std::min(version, 4u)));
        output->scale = 1.0f;
        wl_output_add_listener(output->output, &kOutputListener, output.get());
        ctx->outputs.push_back(std::move(output));
    } else if (std::strcmp(interface,
                           zwlr_screencopy_manager_v1_interface.name) == 0) {
        ctx->manager =
            static_cast<zwlr_screencopy_manager_v1*>(wl_registry_bind(
                registry, name, &zwlr_screencopy_manager_v1_interface,
                std::min(version, 3u)));
    } else if (std::strcmp(interface, zxdg_output_manager_v1_interface.name) ==
               0) {
        ctx->xdgOutputManager = static_cast<zxdg_output_manager_v1*>(
            wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface,
                             std::min(version, 3u)));
    }
}

void registryGlobalRemove(void*, wl_registry*, uint32_t) {}

const wl_registry_listener kRegistryListener = {registryGlobal,
                                                registryGlobalRemove};

// ── 初始化 / 清理 ──

bool initContext(WlrContext& ctx) {
    ctx.display = wl_display_connect(nullptr);
    if (!ctx.display) {
        Logger::error("wlr: failed to connect to Wayland display");
        return false;
    }
    ctx.registry = wl_display_get_registry(ctx.display);
    wl_registry_add_listener(ctx.registry, &kRegistryListener, &ctx);
    wl_display_roundtrip(ctx.display);

    if (ctx.xdgOutputManager) {
        for (auto& outputPtr : ctx.outputs) {
            auto& output = *outputPtr;
            output.xdg = zxdg_output_manager_v1_get_xdg_output(
                ctx.xdgOutputManager, output.output);
            zxdg_output_v1_add_listener(output.xdg, &kXdgOutputListener,
                                        &output);
        }
        wl_display_roundtrip(ctx.display);
    }

    if (!ctx.outputs.empty()) {
        ctx.outputs[0]->primary = true;
    }
    return true;
}

void cleanupContext(WlrContext& ctx) {
    for (auto& outputPtr : ctx.outputs) {
        auto& output = *outputPtr;
        if (output.xdg) {
            zxdg_output_v1_destroy(output.xdg);
        }
        if (output.output) {
            wl_output_destroy(output.output);
        }
    }
    if (ctx.xdgOutputManager) {
        zxdg_output_manager_v1_destroy(ctx.xdgOutputManager);
    }
    if (ctx.manager) {
        zwlr_screencopy_manager_v1_destroy(ctx.manager);
    }
    if (ctx.shm) {
        wl_shm_destroy(ctx.shm);
    }
    if (ctx.registry) {
        wl_registry_destroy(ctx.registry);
    }
    if (ctx.display) {
        wl_display_disconnect(ctx.display);
    }
}

// ── 截取单个 output 为 RawImage（RGB 格式） ──

bool captureOutputToRawImage(WlrContext& ctx, wl_output* output,
                             RawImage& out) {
    FrameCapture capture;
    capture.shm = ctx.shm;
    capture.frame =
        zwlr_screencopy_manager_v1_capture_output(ctx.manager, 0, output);
    zwlr_screencopy_frame_v1_add_listener(capture.frame, &kFrameListener,
                                          &capture);

    while (!capture.ready && !capture.failed) {
        wl_display_dispatch(ctx.display);
    }

    bool ok = false;
    if (capture.failed || !capture.buffer.data) {
        Logger::error("wlr: frame capture failed");
    } else if (capture.format != WL_SHM_FORMAT_ARGB8888 &&
               capture.format != WL_SHM_FORMAT_XRGB8888) {
        Logger::error(
            std::format("wlr: unsupported shm format {}", capture.format));
    } else {
        int width = static_cast<int>(capture.width);
        int height = static_cast<int>(capture.height);
        out.width = width;
        out.height = height;
        // C3 使用 RGB（3 字节/像素），从 ARGB/XRGB 转换
        out.pixels.resize(static_cast<size_t>(width) *
                          static_cast<size_t>(height) * 3u);

        const auto* src = static_cast<const uint8_t*>(capture.buffer.data);
        for (int y = 0; y < height; ++y) {
            int srcY = capture.yInvert ? (height - 1 - y) : y;
            const auto* row = reinterpret_cast<const uint32_t*>(
                src + static_cast<size_t>(capture.stride) * srcY);
            for (int x = 0; x < width; ++x) {
                uint32_t pixel = row[x];
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = (pixel >> 0) & 0xFF;

                size_t idx =
                    (static_cast<size_t>(y) * static_cast<size_t>(width) +
                     static_cast<size_t>(x)) *
                    3u;
                out.pixels[idx + 0] = r;
                out.pixels[idx + 1] = g;
                out.pixels[idx + 2] = b;
            }
        }
        ok = true;
    }

    if (capture.buffer.buffer) {
        wl_buffer_destroy(capture.buffer.buffer);
    }
    if (capture.buffer.data) {
        munmap(capture.buffer.data, capture.buffer.size);
    }
    if (capture.frame) {
        zwlr_screencopy_frame_v1_destroy(capture.frame);
    }
    return ok;
}

}  // namespace

// ── 公开 API ──

namespace WlrScreenCapturer {

bool isAvailable() {
    if (!std::getenv("WAYLAND_DISPLAY")) {
        return false;
    }
    WlrContext ctx;
    if (!initContext(ctx)) {
        return false;
    }
    bool ok = ctx.manager != nullptr;
    cleanupContext(ctx);
    return ok;
}

std::vector<RawImage> captureAllMonitors() {
    std::vector<RawImage> images;

    WlrContext ctx;
    if (!initContext(ctx)) {
        return images;
    }
    if (!ctx.manager || !ctx.shm) {
        Logger::error("wlr: missing screencopy manager or shm");
        cleanupContext(ctx);
        return images;
    }

    for (size_t i = 0; i < ctx.outputs.size(); ++i) {
        auto& outputInfo = *ctx.outputs[i];
        Logger::info(std::format(
            "wlr: capturing output {} '{}' {}x{} @ ({},{})", i, outputInfo.name,
            outputInfo.w, outputInfo.h, outputInfo.x, outputInfo.y));

        RawImage img;
        if (captureOutputToRawImage(ctx, outputInfo.output, img)) {
            images.push_back(std::move(img));
        } else {
            Logger::warn(std::format("wlr: failed to capture output {}", i));
        }
    }

    cleanupContext(ctx);
    return images;
}

}  // namespace WlrScreenCapturer
