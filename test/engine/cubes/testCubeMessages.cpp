/**
 * File: testCubeMessages.cpp
 * 
 * Author: Matt Michini
 * Created: 2018-03-12
 * 
 * Description: Unit tests for cube messaging - compares CLAD messages with cube firmware message definitions
 * 
 * Copyright: Anki, Inc. 2018
 * 
 * --gtest_filter=CubeMessages
 * 
 **/

#include "gtest/gtest.h"

#include "clad/externalInterface/cubeMessages.h"

#include "robot/cube_firmware/app/protocol.h"

using namespace Anki::Cozmo;

// Ensure that CLAD-defined cube messages match those used in the cube firmware
TEST(CubeMessages, CompareTags)
{
  // Ensure that the CLAD message tag type matches the CubeCommand (which is the command type byte in firmware)
  const bool typesMatch = std::is_same<std::underlying_type<CubeMessageTag>::type, CubeCommand>::value;
  ASSERT_TRUE(typesMatch);
  
  // Compare message tags
  EXPECT_EQ(static_cast<uint8_t>(CubeMessageTag::lightSequence),  COMMAND_LIGHT_INDEX);
  EXPECT_EQ(static_cast<uint8_t>(CubeMessageTag::lightKeyframes), COMMAND_LIGHT_KEYFRAMES);
  EXPECT_EQ(static_cast<uint8_t>(CubeMessageTag::accelData),      COMMAND_ACCEL_DATA);
}

TEST(CubeMessages, CompareSizes)
{
  // Base message union
  CubeMessage message;
  
  // CubeLightSequence (which is MapCommand in cube firmware)
  message.Set_lightSequence(CubeLightSequence());
  MapCommand mapCommand;
  EXPECT_EQ(message.Size(), sizeof(mapCommand));

  // CubeLightKeyframeChunk (which is FrameCommand in cube firmware)
  message.Set_lightKeyframes(CubeLightKeyframeChunk());
  FrameCommand frameCommand;
  EXPECT_EQ(message.Size(), sizeof(frameCommand));
  
  // CubeAccelData (which is TapCommand in cube firmware)
  message.Set_accelData(CubeAccelData());
  TapCommand tapCommand;
  EXPECT_EQ(message.Size(), sizeof(tapCommand));
}

TEST(CubeMessages, TestConstants)
{
  CubeLightSequence lightSequenceMsg;
  EXPECT_EQ(ANIMATION_CHANNELS, lightSequenceMsg.initialIndex.size());
  
  CubeLightKeyframeChunk keyframeChunkMsg;
  EXPECT_EQ(FRAMES_PER_COMMAND, keyframeChunkMsg.keyframes.size());
  
  EXPECT_EQ(3, COLOR_CHANNELS);
}
