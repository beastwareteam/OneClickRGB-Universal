#pragma once

#include "../../core/Types.h"
#include <vector>
#include <cstddef>
#include <cstdint>

namespace OCRGB {
namespace App {

using EffectFrame = std::vector<RGB>;

struct EffectSequence2D {
    std::vector<EffectFrame> frameZoneMatrix;
    uint32_t frequencyHz = 30;
    bool loop = true;
};

class EffectFactory {
public:
    static EffectSequence2D CreateStatic(const RGB& color, size_t zoneCount);

    static EffectSequence2D CreateFlowGradient(
        size_t zoneCount,
        size_t frameCount,
        const RGB& startColor,
        const RGB& endColor,
        uint32_t frequencyHz);

    static const EffectFrame& SelectFrame(const EffectSequence2D& sequence, uint64_t tickIndex);

private:
    static RGB Interpolate(const RGB& startColor, const RGB& endColor, float t);
};

} // namespace App
} // namespace OCRGB
