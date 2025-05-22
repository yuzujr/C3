#ifndef IMAGE_ENCODER_H
#define IMAGE_ENCODER_H

#include <cstdint>
#include <vector>

#include "core/RawImage.h"

class ImageEncoder {
public:
    // 禁止创建实例
    ImageEncoder() = delete;

    // 编码 RawImage 为 JPEG 格式
    static std::vector<uint8_t> encodeToJPEG(const RawImage& image,
                                             int quality = 90);
};

#endif  // IMAGE_ENCODER_H