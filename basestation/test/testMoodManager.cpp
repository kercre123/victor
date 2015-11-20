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
#include "anki/cozmo/basestation/moodSystem/emotionAffector.h"
#include "anki/cozmo/basestation/moodSystem/emotionEvent.h"
#include "anki/cozmo/basestation/moodSystem/emotionEventMapper.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/moodSystem/staticMoodData.h"
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

  MoodManager::GetStaticMoodData().SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
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
  
  MoodManager::GetStaticMoodData().SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
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
  
  MoodManager::GetStaticMoodData().SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
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

  MoodManager::GetStaticMoodData().SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  
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


// ================================================================================
// EmotionScorer
// ================================================================================


TEST(MoodManager, EmotionScorerRoundTripJson)
{
  EmotionScorer testScorer(EmotionType::Happy,
                           Anki::Util::GraphEvaluator2d({{-1.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 0.6f}}), true);
  
  EXPECT_EQ(testScorer.GetEmotionType(), EmotionType::Happy);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 3);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(0)._x, -1.0f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(0)._y,  0.0f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(1)._x,  0.5f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(1)._y,  1.0f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(2)._x,  1.0f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(2)._y,  0.6f);
  EXPECT_EQ(testScorer.TrackDeltaScore(), true);
  
  Json::Value testJson;
  const bool writeRes = testScorer.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  EmotionScorer testScorer2(testJson);
  
  EXPECT_EQ(testScorer2.GetEmotionType(), EmotionType::Happy);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNumNodes(), 3);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNode(0)._x, -1.0f);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNode(0)._y,  0.0f);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNode(1)._x,  0.5f);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNode(1)._y,  1.0f);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNode(2)._x,  1.0f);
  EXPECT_EQ(testScorer2.GetScoreGraph().GetNode(2)._y,  0.6f);
  EXPECT_EQ(testScorer2.TrackDeltaScore(), true);
}


static const char* kTestEmotionScorerJson =
"{"
"   \"emotionType\" : \"Calm\","
"   \"scoreGraph\" : {"
"      \"nodes\" : ["
"         {"
"            \"x\" : -1,"
"            \"y\" : -0.5"
"         },"
"         {"
"            \"x\" : 2.0,"
"            \"y\" : 1.5"
"         }"
"      ]"
"   },"
"   \"trackDelta\" : true"
"}";


TEST(MoodManager, EmotionScorerReadJson)
{
  Json::Value  emotionScorerJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestEmotionScorerJson, emotionScorerJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionScorer testScorer(emotionScorerJson);

  EXPECT_EQ(testScorer.GetEmotionType(), EmotionType::Calm);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 2);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(0)._x, -1.0f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(0)._y, -0.5f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(1)._x,  2.0f);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNode(1)._y,  1.5f);
  EXPECT_EQ(testScorer.TrackDeltaScore(), true);
}


// Bad emotionType
static const char* kBadTestEmotionScorerJson =
"{"
"   \"emotionType\" : \"Welsh\","
"   \"scoreGraph\" : {"
"      \"nodes\" : ["
"         {"
"            \"x\" : -1,"
"            \"y\" : -0.5"
"         }"
"      ]"
"   },"
"   \"trackDelta\" : false"
"}";


// Unknown EmotionType entry
TEST(MoodManager, EmotionScorerReadBadJson)
{
  Json::Value  emotionScorerJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBadTestEmotionScorerJson, emotionScorerJson, false);
  ASSERT_TRUE(parsedOK);

  EmotionScorer testScorer(EmotionType::Brave, Anki::Util::GraphEvaluator2d(), false);
  const bool readRes = testScorer.ReadFromJson(emotionScorerJson);
  
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testScorer.GetEmotionType(), EmotionType::Count);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 0);
  EXPECT_EQ(testScorer.TrackDeltaScore(), false);
}


// Missing emotionType entry
static const char* kBad2TestEmotionScorerJson =
"{"
"   \"scoreGraph\" : {"
"      \"nodes\" : ["
"         {"
"            \"x\" : -1,"
"            \"y\" : -0.5"
"         }"
"      ]"
"   },"
"   \"trackDelta\" : false"
"}";


TEST(MoodManager, EmotionScorerReadBadJson2)
{
  Json::Value  emotionScorerJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBad2TestEmotionScorerJson, emotionScorerJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionScorer testScorer(EmotionType::Brave, Anki::Util::GraphEvaluator2d(), false);
  const bool readRes = testScorer.ReadFromJson(emotionScorerJson);
  
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testScorer.GetEmotionType(), EmotionType::Count);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 0);
  EXPECT_EQ(testScorer.TrackDeltaScore(), false);
}


// Missing scoreGraph entry
static const char* kBad3TestEmotionScorerJson =
"{"
"   \"emotionType\" : \"Calm\","
"   \"trackDelta\" : false"
"}";


TEST(MoodManager, EmotionScorerReadBadJson3)
{
  Json::Value  emotionScorerJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBad3TestEmotionScorerJson, emotionScorerJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionScorer testScorer(EmotionType::Brave, Anki::Util::GraphEvaluator2d(), false);
  const bool readRes = testScorer.ReadFromJson(emotionScorerJson);
  
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testScorer.GetEmotionType(), EmotionType::Calm);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 0);
  EXPECT_EQ(testScorer.TrackDeltaScore(), false);
}


// Missing scoreGraph entry
static const char* kBad4TestEmotionScorerJson =
"{"
"   \"emotionType\" : \"Calm\","
"   \"scoreGraph\" : {"
"      \"nodes\" : ["
"         {"
"            \"x\" : -1,"
"            \"y\" : -0.5"
"         }"
"      ]"
"   }"
"}";


TEST(MoodManager, EmotionScorerReadBadJson4)
{
  Json::Value  emotionScorerJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBad4TestEmotionScorerJson, emotionScorerJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionScorer testScorer(EmotionType::Brave, Anki::Util::GraphEvaluator2d(), false);
  const bool readRes = testScorer.ReadFromJson(emotionScorerJson);
  
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testScorer.GetEmotionType(), EmotionType::Calm);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 1);
  EXPECT_EQ(testScorer.TrackDeltaScore(), false);
}


// ================================================================================
// EmotionAffector
// ================================================================================


TEST(MoodManager, EmotionAffectorRoundTripJson)
{
  EmotionAffector testAffector(EmotionType::Happy, 0.75f);
  
  Json::Value testJson;
  const bool writeRes = testAffector.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  EmotionAffector testAffector2;
  const bool readRes = testAffector2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);
  EXPECT_EQ(testAffector2.GetType(),  EmotionType::Happy);
  EXPECT_EQ(testAffector2.GetValue(), 0.75f);
}


// Valid Json
static const char* kTestEmotionAffectorJson =
"{"
"   \"emotionType\" : \"Calm\","
"   \"value\" : 0.33"
"}";


TEST(MoodManager, EmotionAffectorReadJson)
{
  Json::Value  emotionAffectorJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestEmotionAffectorJson, emotionAffectorJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionAffector testAffector;
  const bool readRes = testAffector.ReadFromJson(emotionAffectorJson);
  
  EXPECT_TRUE(readRes);
  EXPECT_EQ(testAffector.GetType(),  EmotionType::Calm);
  EXPECT_EQ(testAffector.GetValue(), 0.33f);
}


// ================================================================================
// EmotionEvent
// ================================================================================


TEST(MoodManager, EmotionEventRoundTripJson)
{
  EmotionEvent testEvent;
  testEvent.SetName("TestEvent1");
  testEvent.AddEmotionAffector(EmotionAffector(EmotionType::Brave, 0.11f));
  testEvent.AddEmotionAffector(EmotionAffector(EmotionType::Happy, 0.22f));
  testEvent.AddEmotionAffector(EmotionAffector(EmotionType::Calm,  0.33f));
  
  Json::Value testJson;
  const bool writeRes = testEvent.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  EmotionEvent testEvent2;
  const bool readRes = testEvent2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);
  
  EXPECT_EQ(testEvent2.GetName(), "TestEvent1");
  EXPECT_EQ(testEvent2.GetNumAffectors(), 3);
  EXPECT_EQ(testEvent2.GetAffector(0).GetType(),  EmotionType::Brave);
  EXPECT_EQ(testEvent2.GetAffector(0).GetValue(), 0.11f);
  EXPECT_EQ(testEvent2.GetAffector(1).GetType(),  EmotionType::Happy);
  EXPECT_EQ(testEvent2.GetAffector(1).GetValue(), 0.22f);
  EXPECT_EQ(testEvent2.GetAffector(2).GetType(),  EmotionType::Calm);
  EXPECT_EQ(testEvent2.GetAffector(2).GetValue(), 0.33f);
}


// Valid Json
static const char* kTestEmotionEventJson =
"{"
"   \"name\" : \"TestEvent1\","
"   \"emotionAffectors\" : ["
"      {"
"         \"emotionType\" : \"Brave\","
"         \"value\" : 0.11"
"      },"
"      {"
"         \"emotionType\" : \"Happy\","
"         \"value\" : 0.22"
"      },"
"      {"
"         \"emotionType\" : \"Calm\","
"         \"value\" : 0.33"
"      }"
"   ]"
"}";


TEST(MoodManager, EmotionEventReadJson)
{
  Json::Value  emotionEventJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestEmotionEventJson, emotionEventJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionEvent testEvent;
  const bool readRes = testEvent.ReadFromJson(emotionEventJson);
  
  EXPECT_TRUE(readRes);
  
  EXPECT_EQ(testEvent.GetName(), "TestEvent1");
  EXPECT_EQ(testEvent.GetNumAffectors(), 3);
  EXPECT_EQ(testEvent.GetAffector(0).GetType(),  EmotionType::Brave);
  EXPECT_EQ(testEvent.GetAffector(0).GetValue(), 0.11f);
  EXPECT_EQ(testEvent.GetAffector(1).GetType(),  EmotionType::Happy);
  EXPECT_EQ(testEvent.GetAffector(1).GetValue(), 0.22f);
  EXPECT_EQ(testEvent.GetAffector(2).GetType(),  EmotionType::Calm);
  EXPECT_EQ(testEvent.GetAffector(2).GetValue(), 0.33f);
}


// ================================================================================
// EmotionEventMapper
// ================================================================================


TEST(MoodManager, EmotionEventMapperRoundTripJson)
{
  EmotionEventMapper testEventMapper;
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent1");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Brave, 0.11f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy, 0.22f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Calm,  0.33f));

    testEventMapper.AddEvent(newEvent);
  }
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent2");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Calm,  0.44f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy, 0.55f));
    
    testEventMapper.AddEvent(newEvent);
  }
  
  {
    // This event will clobber 1st event added
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent1");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Brave, 0.66f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy, 0.77f));
    
    testEventMapper.AddEvent(newEvent);
  }
  
  Json::Value testJson;
  const bool writeRes = testEventMapper.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  EmotionEventMapper testEventMapper2;
  const bool readRes = testEventMapper2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);
  
  {
    const EmotionEvent* foundEvent2 = testEventMapper2.FindEvent("TestEvent2");
    ASSERT_NE(foundEvent2, nullptr);

    EXPECT_EQ(foundEvent2->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetValue(), 0.44f);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetValue(), 0.55f);
    
    const EmotionEvent* foundEvent1 = testEventMapper2.FindEvent("TestEvent1");
    ASSERT_NE(foundEvent1, nullptr);
    
    EXPECT_EQ(foundEvent1->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetType(),  EmotionType::Brave);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetValue(), 0.66f);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetValue(), 0.77f);
  }
}


// Valid Json
static const char* kTestEmotionEventMapperJson =
"{"
"   \"emotionEvents\" : ["
"      {"
"         \"emotionAffectors\" : ["
"            {"
"               \"emotionType\" : \"Happy\","
"               \"value\" : 0.11"
"            },"
"            {"
"               \"emotionType\" : \"Calm\","
"               \"value\" : 0.22"
"            }"
"         ],"
"         \"name\" : \"TestEvent1\""
"      },"
"      {"
"         \"emotionAffectors\" : ["
"            {"
"               \"emotionType\" : \"Brave\","
"               \"value\" : 0.33"
"            },"
"            {"
"               \"emotionType\" : \"Confident\","
"               \"value\" : 0.44"
"            }"
"         ],"
"         \"name\" : \"TestEvent2\""
"      }"
"   ]"
"}";


TEST(MoodManager, EmotionEventMapperReadJson)
{
  Json::Value  emotionEventMapperJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestEmotionEventMapperJson, emotionEventMapperJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionEventMapper testEventMapper;
  const bool readRes = testEventMapper.ReadFromJson(emotionEventMapperJson);
  
  EXPECT_TRUE(readRes);
  
  {
    const EmotionEvent* foundEvent1 = testEventMapper.FindEvent("TestEvent1");
    ASSERT_NE(foundEvent1, nullptr);
    
    EXPECT_EQ(foundEvent1->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetValue(), 0.11f);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetValue(), 0.22f);
    
    const EmotionEvent* foundEvent2 = testEventMapper.FindEvent("TestEvent2");
    ASSERT_NE(foundEvent2, nullptr);
    
    EXPECT_EQ(foundEvent2->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetType(),  EmotionType::Brave);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetValue(), 0.33f);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetType(),  EmotionType::Confident);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetValue(), 0.44f);
  }
}


// ================================================================================
// StaticMoodData
// ================================================================================


TEST(MoodManager, StaticMoodDataRoundTripJson)
{
  StaticMoodData testStaticData;
  
  Anki::Util::GraphEvaluator2d kTestNoDecayGraph({{0.0f, 1.0f}, {1.0f, 1.0f}});
  
  testStaticData.InitDecayGraphs();
  testStaticData.SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  testStaticData.SetDecayGraph(EmotionType::Calm,  kTestNoDecayGraph);
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent1");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Brave,  0.11f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy, -0.22f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Calm,   0.33f));
  
    testStaticData.GetEmotionEventMapper().AddEvent(newEvent);
  }
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent2");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Calm,  -0.44f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy,  0.55f));
    
    testStaticData.GetEmotionEventMapper().AddEvent(newEvent);
  }

  Json::Value testJson;
  const bool writeRes = testStaticData.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  StaticMoodData testStaticData2;
  const bool readRes = testStaticData2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);
  
  {
    const Anki::Util::GraphEvaluator2d& happyGraph = testStaticData2.GetDecayGraph(EmotionType::Happy);
    ASSERT_EQ(happyGraph.GetNumNodes(), kTestDecayGraph.GetNumNodes());
    for (int i=0; i < kTestDecayGraph.GetNumNodes(); ++i)
    {
      EXPECT_EQ(happyGraph.GetNode(i)._x, kTestDecayGraph.GetNode(i)._x);
      EXPECT_EQ(happyGraph.GetNode(i)._y, kTestDecayGraph.GetNode(i)._y);
    }
  }
  
  {
    const Anki::Util::GraphEvaluator2d& calmGraph = testStaticData2.GetDecayGraph(EmotionType::Calm);
    ASSERT_EQ(calmGraph.GetNumNodes(), kTestNoDecayGraph.GetNumNodes());
    for (int i=0; i < kTestNoDecayGraph.GetNumNodes(); ++i)
    {
      EXPECT_EQ(calmGraph.GetNode(i)._x, kTestNoDecayGraph.GetNode(i)._x);
      EXPECT_EQ(calmGraph.GetNode(i)._y, kTestNoDecayGraph.GetNode(i)._y);
    }
  }
  
  {
    const EmotionEvent* foundEvent1 = testStaticData2.GetEmotionEventMapper().FindEvent("TestEvent1");
    ASSERT_NE(foundEvent1, nullptr);
    
    EXPECT_EQ(foundEvent1->GetNumAffectors(), 3);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetType(),  EmotionType::Brave);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetValue(),  0.11f);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetValue(), -0.22f);
    EXPECT_EQ(foundEvent1->GetAffector(2).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent1->GetAffector(2).GetValue(),  0.33f);
    
    const EmotionEvent* foundEvent2 = testStaticData2.GetEmotionEventMapper().FindEvent("TestEvent2");
    ASSERT_NE(foundEvent2, nullptr);
    
    EXPECT_EQ(foundEvent2->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetValue(), -0.44f);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetValue(),  0.55f);
  }
}


// Valid Json
static const char* kTestStaticMoodDataJson =
"{"
"   \"decayGraphs\" : ["
"      {"
"         \"emotionType\" : \"Happy\","
"         \"nodes\" : ["
"            {"
"               \"x\" : 0,"
"               \"y\" : 1"
"            },"
"            {"
"               \"x\" : 1.0,"
"               \"y\" : 0.95"
"            },"
"            {"
"               \"x\" : 2.0,"
"               \"y\" : 0.5"
"            }"
"         ]"
"      },"
"      {"
"         \"emotionType\" : \"Calm\","
"         \"nodes\" : ["
"            {"
"               \"x\" : 0,"
"               \"y\" : 1"
"            },"
"            {"
"               \"x\" : 1.5,"
"               \"y\" : 0.8"
"            }"
"         ]"
"      }"
"   ],"
"   \"eventMapper\" : {"
"      \"emotionEvents\" : ["
"          {"
"             \"emotionAffectors\" : ["
"                {"
"                   \"emotionType\" : \"Brave\","
"                   \"value\" : 0.11"
"                },"
"                {"
"                   \"emotionType\" : \"Happy\","
"                   \"value\" : -0.22"
"                },"
"                {"
"                   \"emotionType\" : \"Calm\","
"                   \"value\" : 0.33"
"                }"
"             ],"
"             \"name\" : \"TestEvent1\""
"          },"
"          {"
"             \"emotionAffectors\" : ["
"                {"
"                   \"emotionType\" : \"Calm\","
"                   \"value\" : -0.44"
"                },"
"                {"
"                   \"emotionType\" : \"Happy\","
"                   \"value\" : 0.55"
"                }"
"             ],"
"             \"name\" : \"TestEvent2\""
"          }"
"       ]"
"    }"
"}";


TEST(MoodManager, StaticMoodDataReadJson)
{
  Json::Value  staticMoodDataJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestStaticMoodDataJson, staticMoodDataJson, false);
  ASSERT_TRUE(parsedOK);
  
  StaticMoodData& testStaticMoodData = MoodManager::GetStaticMoodData();
  const bool readRes = testStaticMoodData.ReadFromJson(staticMoodDataJson);
  
  EXPECT_TRUE(readRes);
  
  {
    const Anki::Util::GraphEvaluator2d& happyGraph = testStaticMoodData.GetDecayGraph(EmotionType::Happy);
    ASSERT_EQ(happyGraph.GetNumNodes(), 3);

    EXPECT_FLOAT_EQ(happyGraph.GetNode(2)._x, 2.0f);
    EXPECT_FLOAT_EQ(happyGraph.GetNode(2)._y, 0.5f);
  }
  
  {
    const Anki::Util::GraphEvaluator2d& calmGraph = testStaticMoodData.GetDecayGraph(EmotionType::Calm);
    ASSERT_EQ(calmGraph.GetNumNodes(), 2);

    EXPECT_FLOAT_EQ(calmGraph.GetNode(1)._x, 1.5f);
    EXPECT_FLOAT_EQ(calmGraph.GetNode(1)._y, 0.8f);
  }

  MoodManager moodManager;
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Brave), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.0f);
  moodManager.TriggerEmotionEvent("TestEvent1");
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Brave),  0.11f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -0.22f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),   0.33f);
  moodManager.TriggerEmotionEvent("TestEvent2");
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Brave),  0.11f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Happy),  0.33f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  -0.11f);
}


