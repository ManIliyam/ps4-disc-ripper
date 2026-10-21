#pragma once
#include <cstdint>

// FreeType grayscale coverage is unsigned, including opaque pixels (255).
inline uint8_t fontBlend(uint8_t background, uint8_t foreground, uint8_t coverage) {
    return static_cast<uint8_t>((background * (255u-coverage) + foreground * coverage + 127u)/255u);
}
