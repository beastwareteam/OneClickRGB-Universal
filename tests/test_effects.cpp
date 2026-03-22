//=============================================================================
// Tests for app/effects/EffectFactory
//=============================================================================

#include "TestFramework.h"
#include "../src/app/effects/EffectFactory.h"

using namespace OCRGB;
using namespace OCRGB::App;

//-----------------------------------------------------------------------------
// CreateStatic Tests
//-----------------------------------------------------------------------------

TEST_CASE("EffectFactory: CreateStatic single zone") {
    RGB color(255, 0, 128);
    auto seq = EffectFactory::CreateStatic(color, 1);

    CHECK_EQ(seq.frameZoneMatrix.size(), 1u);
    CHECK_EQ(seq.frameZoneMatrix[0].size(), 1u);
    CHECK(seq.frameZoneMatrix[0][0] == color);
    CHECK(seq.loop);
    PASS();
}

TEST_CASE("EffectFactory: CreateStatic multiple zones") {
    RGB color(0, 255, 0);
    auto seq = EffectFactory::CreateStatic(color, 4);

    CHECK_EQ(seq.frameZoneMatrix.size(), 1u);
    CHECK_EQ(seq.frameZoneMatrix[0].size(), 4u);

    for (const auto& zoneColor : seq.frameZoneMatrix[0]) {
        CHECK(zoneColor == color);
    }
    PASS();
}

//-----------------------------------------------------------------------------
// CreateFlowGradient Tests
//-----------------------------------------------------------------------------

TEST_CASE("EffectFactory: CreateFlowGradient basic") {
    RGB start(255, 0, 0);
    RGB end(0, 0, 255);

    auto seq = EffectFactory::CreateFlowGradient(2, 10, start, end, 30);

    CHECK_EQ(seq.frameZoneMatrix.size(), 10u);
    CHECK_EQ(seq.frequencyHz, 30u);
    CHECK(seq.loop);

    // First frame should be closer to start color
    // Last frame should be closer to end color
    CHECK(seq.frameZoneMatrix[0][0].r > seq.frameZoneMatrix[9][0].r);
    CHECK(seq.frameZoneMatrix[0][0].b < seq.frameZoneMatrix[9][0].b);
    PASS();
}

TEST_CASE("EffectFactory: CreateFlowGradient zero zones") {
    RGB start(255, 0, 0);
    RGB end(0, 0, 255);

    auto seq = EffectFactory::CreateFlowGradient(0, 10, start, end, 30);

    // Should handle gracefully
    CHECK_EQ(seq.frameZoneMatrix.size(), 1u);
    PASS();
}

TEST_CASE("EffectFactory: CreateFlowGradient zero frames") {
    RGB start(255, 0, 0);
    RGB end(0, 0, 255);

    auto seq = EffectFactory::CreateFlowGradient(4, 0, start, end, 30);

    // Should handle gracefully
    CHECK_EQ(seq.frameZoneMatrix.size(), 1u);
    PASS();
}

TEST_CASE("EffectFactory: CreateFlowGradient frequency clamped") {
    RGB start(255, 0, 0);
    RGB end(0, 0, 255);

    auto seq = EffectFactory::CreateFlowGradient(2, 5, start, end, 0);

    // Frequency should be at least 1
    CHECK(seq.frequencyHz >= 1);
    PASS();
}

//-----------------------------------------------------------------------------
// SelectFrame Tests
//-----------------------------------------------------------------------------

TEST_CASE("EffectFactory: SelectFrame looping") {
    RGB color(100, 150, 200);
    auto seq = EffectFactory::CreateStatic(color, 2);

    // Add more frames manually for testing
    seq.frameZoneMatrix.push_back({RGB(200, 100, 50), RGB(200, 100, 50)});
    seq.frameZoneMatrix.push_back({RGB(50, 200, 100), RGB(50, 200, 100)});

    CHECK_EQ(seq.frameZoneMatrix.size(), 3u);

    // Frame selection should wrap
    const auto& frame0 = EffectFactory::SelectFrame(seq, 0);
    const auto& frame1 = EffectFactory::SelectFrame(seq, 1);
    const auto& frame2 = EffectFactory::SelectFrame(seq, 2);
    const auto& frame3 = EffectFactory::SelectFrame(seq, 3); // Should wrap to 0

    CHECK(frame0[0] == color);
    CHECK(frame3[0] == color); // Same as frame 0
    PASS();
}

TEST_CASE("EffectFactory: SelectFrame non-looping") {
    RGB color(100, 150, 200);
    auto seq = EffectFactory::CreateStatic(color, 2);
    seq.loop = false;

    // Add more frames
    seq.frameZoneMatrix.push_back({RGB(200, 100, 50), RGB(200, 100, 50)});

    // Beyond last frame should clamp to last
    const auto& frameLast = EffectFactory::SelectFrame(seq, 100);
    CHECK(frameLast[0] == RGB(200, 100, 50));
    PASS();
}

TEST_CASE("EffectFactory: SelectFrame empty sequence") {
    EffectSequence2D emptySeq;
    emptySeq.frameZoneMatrix.clear();

    const auto& frame = EffectFactory::SelectFrame(emptySeq, 0);

    // Should return empty frame, not crash
    CHECK(frame.empty());
    PASS();
}

//-----------------------------------------------------------------------------
// Interpolate Tests (via gradient inspection)
//-----------------------------------------------------------------------------

TEST_CASE("EffectFactory: gradient interpolation midpoint") {
    RGB start(0, 0, 0);
    RGB end(100, 100, 100);

    // 3 frames: start, middle, end
    auto seq = EffectFactory::CreateFlowGradient(1, 3, start, end, 30);

    // Middle frame should be approximately midpoint
    const auto& midFrame = seq.frameZoneMatrix[1];
    CHECK(midFrame[0].r >= 20 && midFrame[0].r <= 80);
    CHECK(midFrame[0].g >= 20 && midFrame[0].g <= 80);
    CHECK(midFrame[0].b >= 20 && midFrame[0].b <= 80);
    PASS();
}
