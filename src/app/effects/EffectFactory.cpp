#include "EffectFactory.h"
#include <algorithm>
#include <utility>

namespace OCRGB {
namespace App {

EffectSequence2D EffectFactory::CreateStatic(const RGB& color, size_t zoneCount) {
    EffectSequence2D sequence;
    sequence.frequencyHz = 1;
    sequence.loop = true;
    sequence.frameZoneMatrix = {EffectFrame(zoneCount, color)};
    return sequence;
}

EffectSequence2D EffectFactory::CreateFlowGradient(
    size_t zoneCount,
    size_t frameCount,
    const RGB& startColor,
    const RGB& endColor,
    uint32_t frequencyHz) {

    EffectSequence2D sequence;
    sequence.frequencyHz = std::max<uint32_t>(1, frequencyHz);
    sequence.loop = true;

    if (zoneCount == 0 || frameCount == 0) {
        sequence.frameZoneMatrix = {EffectFrame()};
        return sequence;
    }

    sequence.frameZoneMatrix.reserve(frameCount);

    for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        float frameBlend = static_cast<float>(frameIndex) / static_cast<float>(std::max<size_t>(1, frameCount - 1));

        EffectFrame frame;
        frame.reserve(zoneCount);

        for (size_t zoneIndex = 0; zoneIndex < zoneCount; ++zoneIndex) {
            float zonePhase = static_cast<float>(zoneIndex) / static_cast<float>(std::max<size_t>(1, zoneCount - 1));
            float blend = std::clamp((frameBlend + zonePhase) * 0.5f, 0.0f, 1.0f);
            frame.push_back(Interpolate(startColor, endColor, blend));
        }

        sequence.frameZoneMatrix.push_back(std::move(frame));
    }

    return sequence;
}

const EffectFrame& EffectFactory::SelectFrame(const EffectSequence2D& sequence, uint64_t tickIndex) {
    static EffectFrame emptyFrame;

    if (sequence.frameZoneMatrix.empty()) {
        return emptyFrame;
    }

    size_t frameCount = sequence.frameZoneMatrix.size();
    size_t selectedIndex = 0;

    if (sequence.loop) {
        selectedIndex = static_cast<size_t>(tickIndex % frameCount);
    } else {
        selectedIndex = static_cast<size_t>(std::min<uint64_t>(tickIndex, frameCount - 1));
    }

    return sequence.frameZoneMatrix[selectedIndex];
}

RGB EffectFactory::Interpolate(const RGB& startColor, const RGB& endColor, float t) {
    auto lerpChannel = [t](uint8_t from, uint8_t to) -> uint8_t {
        float value = static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * t;
        value = std::clamp(value, 0.0f, 255.0f);
        return static_cast<uint8_t>(value);
    };

    return RGB(
        lerpChannel(startColor.r, endColor.r),
        lerpChannel(startColor.g, endColor.g),
        lerpChannel(startColor.b, endColor.b));
}

} // namespace App
} // namespace OCRGB
