/**
 * File: testLedAnimation.cpp
 * 
 * Author: Matt Michini
 * Created: 2018-03-12
 * 
 * Description: Unit tests for cube LED animations
 * 
 * Copyright: Anki, Inc. 2018
 * 
 * --gtest_filter=CubeLedAnimation
 * 
 **/

#include "gtest/gtest.h"

#include "clad/externalInterface/messageEngineToCube.h"
#include "clad/types/ledTypes.h"

#include "engine/components/cubes/ledAnimation.h"

using namespace Anki::Vector;

TEST(CubeLedAnimation, TestAllFields)
{
  LightState lightState;
  
  lightState.onColor   = 0xAABBCCDD;
  lightState.offColor  = 0x11223344;
  lightState.onFrames  = 10;
  lightState.offFrames = 20;
  lightState.transitionOnFrames  = 30;
  lightState.transitionOffFrames = 40;
  lightState.offset = 0;
  
  std::vector<int> baseIndices = {{0, 5}};
  for (auto baseIndex : baseIndices) {
    LedAnimation ledAnimation(lightState, baseIndex);
    
    const auto& keyframes = ledAnimation.GetKeyframes();
    
    // Should have 3 keyframes
    ASSERT_EQ(3, keyframes.size());
    
    // Starting index should be baseIndex + 0, and end index should be baseIndex + 2
    EXPECT_EQ(baseIndex + 0, ledAnimation.GetStartingIndex());
    EXPECT_EQ(baseIndex + 2, ledAnimation.GetFinalIndex());
    
    // Keyframe 0 should be offColor, with holdFrames = 0, decayFrames = transitionOnFrames, nextIndex = baseIndex + 1
    EXPECT_EQ(0x11,                          keyframes[0].color[0]);
    EXPECT_EQ(0x22,                          keyframes[0].color[1]);
    EXPECT_EQ(0x33,                          keyframes[0].color[2]);
    EXPECT_EQ(0,                             keyframes[0].holdFrames);
    EXPECT_EQ(lightState.transitionOnFrames, keyframes[0].decayFrames);
    EXPECT_EQ(baseIndex + 1,                 keyframes[0].nextIndex);
    
    // Keyframe 1 should be onColor, with holdFrames = onFrames, decayFrames = transitionOffFrames, nextIndex = baseIndex + 2
    EXPECT_EQ(0xAA,                           keyframes[1].color[0]);
    EXPECT_EQ(0xBB,                           keyframes[1].color[1]);
    EXPECT_EQ(0xCC,                           keyframes[1].color[2]);
    EXPECT_EQ(lightState.onFrames,            keyframes[1].holdFrames);
    EXPECT_EQ(lightState.transitionOffFrames, keyframes[1].decayFrames);
    EXPECT_EQ(baseIndex + 2,                  keyframes[1].nextIndex);
    
    // Keyframe 2 should be offColor, with holdFrames = offTime, decayFrames = 0, nextIndex = baseIndex
    EXPECT_EQ(0x11,                 keyframes[2].color[0]);
    EXPECT_EQ(0x22,                 keyframes[2].color[1]);
    EXPECT_EQ(0x33,                 keyframes[2].color[2]);
    EXPECT_EQ(lightState.offFrames, keyframes[2].holdFrames);
    EXPECT_EQ(0,                    keyframes[2].decayFrames);
    EXPECT_EQ(baseIndex,            keyframes[2].nextIndex);
  }
}


TEST(CubeLedAnimation, TestOffset)
{
  LightState lightState;
  
  lightState.onColor   = 0xAABBCCDD;
  lightState.offColor  = 0x11223344;
  lightState.onFrames  = 10;
  lightState.offFrames = 20;
  lightState.transitionOnFrames  = 30;
  lightState.transitionOffFrames = 40;
  lightState.offset = 50;
  
  std::vector<int> baseIndices = {{0, 9}};
  for (auto baseIndex : baseIndices) {
    LedAnimation ledAnimation(lightState, baseIndex);
    
    const auto& keyframes = ledAnimation.GetKeyframes();
    
    // Should have 4 keyframes
    ASSERT_EQ(4, keyframes.size());
    
    // Starting index should be baseIndex + 0, and end index should be baseIndex + 3
    EXPECT_EQ(baseIndex + 0, ledAnimation.GetStartingIndex());
    EXPECT_EQ(baseIndex + 3, ledAnimation.GetFinalIndex());
    
    // Keyframe 0 should be black, with holdFrames = offset, decayFrames = 0, nextIndex = baseIndex + 1
    EXPECT_EQ(0,                 keyframes[0].color[0]);
    EXPECT_EQ(0,                 keyframes[0].color[1]);
    EXPECT_EQ(0,                 keyframes[0].color[2]);
    EXPECT_EQ(lightState.offset, keyframes[0].holdFrames);
    EXPECT_EQ(0,                 keyframes[0].decayFrames);
    EXPECT_EQ(baseIndex + 1,     keyframes[0].nextIndex);
  }
}


TEST(CubeLedAnimation, TestPlayOnce)
{
  LightState lightState;
  
  lightState.onColor   = 0xAABBCCDD;
  lightState.offColor  = 0x11223344;
  lightState.onFrames  = 10;
  lightState.offFrames = 20;
  lightState.transitionOnFrames  = 30;
  lightState.transitionOffFrames = 40;
  lightState.offset = 0;
  
  std::vector<int> baseIndices = {{0, 13}};
  for (auto baseIndex : baseIndices) {
    LedAnimation ledAnimation(lightState, baseIndex);
    
    ledAnimation.SetPlayOnce();
    
    const auto& keyframes = ledAnimation.GetKeyframes();
    
    // Should have 3 keyframes
    ASSERT_EQ(3, keyframes.size());
    
    // Starting index should be baseIndex + 0, and end index should be baseIndex + 2
    EXPECT_EQ(baseIndex + 0, ledAnimation.GetStartingIndex());
    EXPECT_EQ(baseIndex + 2, ledAnimation.GetFinalIndex());
    
    // Keyframe 2 should be offColor, with holdFrames = 0, decayFrames = 0, nextIndex = baseIndex
    EXPECT_EQ(0x11, keyframes[2].color[0]);
    EXPECT_EQ(0x22, keyframes[2].color[1]);
    EXPECT_EQ(0x33, keyframes[2].color[2]);
    EXPECT_EQ(0,    keyframes[2].holdFrames);
    EXPECT_EQ(0,    keyframes[2].decayFrames);
  }
}


TEST(CubeLedAnimation, TestLinkToOther)
{
  LightState lightState;
  
  lightState.onColor   = 0xAABBCCDD;
  lightState.offColor  = 0x11223344;
  lightState.onFrames  = 10;
  lightState.offFrames = 20;
  lightState.transitionOnFrames  = 30;
  lightState.transitionOffFrames = 40;
  lightState.offset = 0;

  LedAnimation first(lightState, 0);
  LedAnimation second(lightState, 3);
  first.LinkToOther(second);
  
  const auto& keyframesFirst = first.GetKeyframes();
  ASSERT_EQ(3, keyframesFirst.size());
  
  const auto& keyframesSecond = second.GetKeyframes();
  ASSERT_EQ(3, keyframesSecond.size());
  
  EXPECT_EQ(keyframesFirst[first.GetFinalIndex()].nextIndex, second.GetStartingIndex());
}

