/**
 * File: testMoodManager
 *
 * Author: Mark Wesley
 * Created: 10/14/15
 *
 * Description: Unit tests for Mood Manager and Emotions
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=MoodManager*
 **/


#include "gtest/gtest.h"

#include "util/math/math.h"
#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include <assert.h>


using namespace Anki::Cozmo;


const double kTickTimestep = 0.01;
double gCurrentTime = 0.0; // Fake time for update calls

Anki::Util::GraphEvaluator2d kTestDecayGraph({{0.0f, 1.0f }, {0.5f, 1.0f }, {1.0f, 0.9f }, {1.5f, 0.8f },
                                              {2.0f, 0.5f }, {2.5f, 0.5f }, {3.0f, 0.4f }, {3.5f, 0.0f } });


// Helper - tick mood manager for N ticks with a given timestep
void TickMoodManager(MoodManager& moodManager, uint32_t numTicks, float tickTimeStep)
{
  for (uint32_t i=0; i < numTicks; ++i)
  {
    gCurrentTime += tickTimeStep;
    moodManager.Update(gCurrentTime);
  }
}


TEST(MoodManager, AddEmotionNoPenalty)
{
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 1),   0.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1");
  moodManager.AddToEmotion(EmotionType::Calm,  0.1f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 1),   0.1f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  1.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  1.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -0.5f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1), -0.5f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1), -1.0f, 0.0f);
}


TEST(MoodManager, AddEmotionNoPenaltyEachTick)
{
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 99),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm,  1),   0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 99),   0.0f, 0.0f);
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1");
  moodManager.AddToEmotion(EmotionType::Calm,  0.1f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 99),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm,  1),   0.1f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 99),   0.1f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  2),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 99),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm,  1),   0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm,  2),   0.1f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 99),   0.1f, 0.0f);
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  2),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 99),  1.0f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  2),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  3),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 99),  1.0f, 0.0f);
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   2),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   3),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  99),  1.0f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   2),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   3),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   4),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  99),  1.0f, 0.0f);
  
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -0.5f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   1), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   2), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   3), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   4), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  99), -0.5f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   2), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   3), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   4), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   5), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  99), -0.5f, 0.0f);
  
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   1), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   2), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   3), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   4), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   5), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  99), -1.0f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   2), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   3), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   4), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   5), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,   6), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  99), -1.0f, 0.0f);
}


TEST(MoodManager, DecayFromPositive)
{
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);

  MoodManager::SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 10.0f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.1f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f,  0.000001f);

  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 11), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 10), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  0), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.2f   ), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.091f ), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.09f ),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.0f   ), 0.0f, 0.0f);
  
  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.4f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f,  0.000001f);
  
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 41), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 40), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.391f ),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.39f   ), 0.0f, 0.0f);
  
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f,  0.000001f);
  
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 51), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 50), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.491f ),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.49f   ), 0.0f, 0.0f);

  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.8f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.94f, 0.000001f);
  
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 81),  0.94f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 80), -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 31), -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 16), -0.03f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.00f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.791f ),   0.94f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.79f  ),  -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.3f   ),  -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.15f  ),  -0.03f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(EmotionType::Happy, 0.0f   ),   0.0f , 0.0001f);

  TickMoodManager(moodManager, 20, kTickTimestep); // now = 1.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.9f,  0.000001f);
  
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.85f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.8f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.65f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.45f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.4f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.2f,  0.00001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f,  0.00001f);
  // Should remain zero
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 4.0f
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
}


TEST(MoodManager, DecayFromNegative)
{
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  MoodManager::SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, -10.0f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f);
  
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.1f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f,  0.000001f);
  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.4f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f,  0.000001f);
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f,  0.000001f);
  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.8f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.94f, 0.000001f);
  TickMoodManager(moodManager, 20, kTickTimestep); // now = 1.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.9f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.85f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.8f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.65f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.45f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.4f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), -0.2f,  0.00001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy),  0.0f,  0.00001f);
  // Should remain zero
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 4.0f
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
}


TEST(MoodManager, DecayResetFromAwards)
{
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  MoodManager::SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 10.0f, "Test1");
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  
  TickMoodManager(moodManager, 20, 0.1f); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f,  0.000001f);

  moodManager.AddToEmotion(EmotionType::Happy, 0.25f, "Test1");
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.75f, 0.000001f);
  // now = 0.0f (add reset the decay)
  
  TickMoodManager(moodManager, 20, 0.1f); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.375f,  0.1f);

  moodManager.AddToEmotion(EmotionType::Happy, -0.25f, "Test1");
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.125f, 0.000001f);
  // now still = 2.0f (subtract doesn't reset the decay)
  // Note: due to the decrease this is equivalent to a 0.25 initial value dropped to 0.125 after 2 seconds

  TickMoodManager(moodManager, 10, 0.1f); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.1f,  0.0001f);
  
}


class TestBehavior : public IBehavior
{
public:
  
  TestBehavior(Robot& robot, const Json::Value& config, const char* name)
    : IBehavior(robot, config)
  {
    _name = name;
  }

  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override { return true; }

  virtual Anki::Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override { return Anki::RESULT_OK; }
  virtual Status       UpdateInternal(Robot& robot, double currentTime_sec) override { return Status::Running; }
  virtual Anki::Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override { return Anki::RESULT_OK; }
};


TEST(MoodManager, BehaviorScoring)
{
  RobotInterface::MessageHandler messageHandler;
  Robot testRobot(RobotID_t(0), &messageHandler, nullptr, nullptr);

  MoodManager& moodManager = testRobot.GetMoodManager();
  TickMoodManager(moodManager, 1, kTickTimestep);

  MoodManager::SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
  Json::Value config;
  // have to alloc the behaviors - they're freed by the chooser
  TestBehavior* testBehaviorReqHappy = new TestBehavior(testRobot, config, "TestHappy");
  TestBehavior* testBehaviorReqCalm = new TestBehavior(testRobot, config, "TestCalm");
  SimpleBehaviorChooser behaviorChooser;
  behaviorChooser.AddBehavior(testBehaviorReqHappy);
  behaviorChooser.AddBehavior(testBehaviorReqCalm);
  
  testBehaviorReqHappy->AddEmotionScorer(EmotionScorer(EmotionType::Happy, Anki::Util::GraphEvaluator2d({{-1.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 0.6f}}), false));
  testBehaviorReqCalm->AddEmotionScorer( EmotionScorer(EmotionType::Calm,  Anki::Util::GraphEvaluator2d({{-1.0f, 0.5f}, {0.5f, 0.0f}, {1.0f, 0.0f}}), false));
  
  float score1 = testBehaviorReqHappy->EvaluateScore(testRobot, gCurrentTime);
  float score2 = testBehaviorReqCalm->EvaluateScore(testRobot, gCurrentTime);
  
  EXPECT_FLOAT_EQ(score1, 0.6666666666f);
  EXPECT_FLOAT_EQ(score2, 0.16666666f);
  
  {
    IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, gCurrentTime);
    EXPECT_EQ(behaviorChosen, testBehaviorReqHappy);
  }
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.25f, "Test1");
  moodManager.AddToEmotion(EmotionType::Calm,  0.5f, "Test1");
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot, gCurrentTime);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot, gCurrentTime);
  
  EXPECT_FLOAT_EQ(score1, 0.83333331f);
  EXPECT_FLOAT_EQ(score2, 0.0f);
  
  {
    IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, gCurrentTime);
    EXPECT_EQ(behaviorChosen, testBehaviorReqHappy);
  }
  
  moodManager.AddToEmotion(EmotionType::Happy, -2.0f, "Test1");
  moodManager.AddToEmotion(EmotionType::Calm,  -2.0f, "Test1");
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot, gCurrentTime);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot, gCurrentTime);
  
  EXPECT_FLOAT_EQ(score1, 0.0f);
  EXPECT_FLOAT_EQ(score2, 0.5f);
  
  {
    IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, gCurrentTime);
    EXPECT_EQ(behaviorChosen, testBehaviorReqCalm);
  }

  moodManager.AddToEmotion(EmotionType::Happy, 0.75f, "Test1");
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot, gCurrentTime);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot, gCurrentTime);
  
  EXPECT_FLOAT_EQ(score1, 0.5f);
  EXPECT_FLOAT_EQ(score2, 0.5f);
  
  // Equal scores - check that random factor will enable either behavior to be chosen
  {
    uint32_t behaviorCountHappy = 0;
    uint32_t behaviorCountCalm  = 0;
    const uint32_t kNumTests = 1000;

    for (uint32_t i=0; i < kNumTests; ++i)
    {
      IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, gCurrentTime);
      if (behaviorChosen == testBehaviorReqHappy)
      {
        ++behaviorCountHappy;
      }
      if (behaviorChosen == testBehaviorReqCalm)
      {
        ++behaviorCountCalm;
      }
    }
    
    // there's a 0.5^1000 chance of all being the same, which is less than a 1 in 10^300 chance, i.e. almost never
    // Should have close to 500,500, but will vary so we just assert on >0 and print the results
    EXPECT_GT(behaviorCountHappy, 0);
    EXPECT_GT(behaviorCountCalm,  0);
    EXPECT_EQ(behaviorCountHappy + behaviorCountCalm,  kNumTests);
    printf("[MoodManager.BehaviorScoring.Randomess] of %u choices with 2 equal scores, chose %u happy, %u calm\n", kNumTests, behaviorCountHappy, behaviorCountCalm );
  }
}


