#ifndef IMAGE_ENCODER_H
#define IMAGE_ENCODER_H

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

struct RawImage;

namespace ImageEncoder {

// 编码 RawImage 为 JPEG 格式
std::expected<std::vector<uint8_t>, std::string> encodeToJPEG(
    const RawImage& image, int quality = 90);

}  // namespace ImageEncoder

#endif  // IMAGE_ENCODER_H