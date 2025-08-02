#include "core/ImageEncoder.h"

#include "core/RawImage.h"
#include "turbojpeg.h"

namespace ImageEncoder {

std::expected<std::vector<uint8_t>, std::string> encodeToJPEG(
    const RawImage& image, int quality) {
    // 检查 RawImage 是否有效
    if (image.pixels.empty() || image.width <= 0 || image.height <= 0) {
        return std::unexpected(
            "Invalid RawImage input: image is empty or has invalid "
            "dimensions.");
    }

    tjhandle compressor = tjInitCompress();
    if (!compressor) {
        return std::unexpected("Failed to initialize TurboJPEG compressor.");
    }

    unsigned char* jpegBuf = nullptr;
    unsigned long jpegSize = 0;

    // 压缩图像
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

    // 检查输出是否为空
    if (output.empty()) {
        return std::unexpected("Encoded JPEG output is empty.");
    }

    return output;  // 返回编码后的图像数据
}

}  // namespace ImageEncoder