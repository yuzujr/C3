#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include <cstdint>
#include <vector>

struct RawImage {
    RawImage() : width(0), height(0) {}

    int width;
    int height;
    std::vector<uint8_t> pixels;  // RGB format

    bool empty() const;
};

#endif  // RAW_IMAGE_H