#include "core/RawImage.h"

bool RawImage::empty() const {
    return pixels.empty() || width <= 0 || height <= 0;
}