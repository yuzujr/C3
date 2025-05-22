#include "core/ImageEncoder.h"

#include <stdexcept>

#include "turbojpeg.h"

std::vector<uint8_t> ImageEncoder::encodeToJPEG(const RawImage& image,
                                                int quality) {
    if (image.pixels.empty() || image.width <= 0 || image.height <= 0) {
        throw std::invalid_argument("Invalid RawImage input");
    }

    tjhandle compressor = tjInitCompress();
    if (!compressor) {
        throw std::runtime_error("Failed to initialize TurboJPEG compressor");
    }

    unsigned char* jpegBuf = nullptr;
    unsigned long jpegSize = 0;

    int result = tjCompress2(compressor,
                             image.pixels.data(),  // RGB 数据
                             image.width,
                             0,  // pitch = 0 表示连续
                             image.height,
                             TJPF_RGB,  // 输入格式
                             &jpegBuf, &jpegSize,
                             TJSAMP_444,  // 采样率 4:4:4（高质量）
                             quality,
                             TJFLAG_FASTDCT  // 可选：更快压缩
    );

    std::vector<uint8_t> output;
    if (result == 0 && jpegBuf && jpegSize > 0) {
        output.assign(jpegBuf, jpegBuf + jpegSize);
    }

    if (jpegBuf) tjFree(jpegBuf);
    tjDestroy(compressor);

    if (output.empty()) {
        throw std::runtime_error("JPEG encoding failed");
    }

    return output;
}