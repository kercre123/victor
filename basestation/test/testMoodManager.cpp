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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/simpleBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/moodSystem/emotionAffector.h"
#include "anki/cozmo/basestation/moodSystem/emotionEvent.h"
#include "anki/cozmo/basestation/moodSystem/emotionEventMapper.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/moodSystem/staticMoodData.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <assert.h>


using namespace Anki;
using namespace Anki::Cozmo;


const double kTickTimestep = 0.01;
double gCurrentTime = 0.0; // Fake time for update calls

Anki::Util::GraphEvaluator2d kTestDefaultRepetitionPenaltyGraph({{0.0f, 0.0}, {10.0f, 1.0f}}); // no-repetition penalty
  
Anki::Util::GraphEvaluator2d kTestDecayGraph({{0.0f, 1.0f }, {0.5f, 1.0f }, {1.0f, 0.9f }, {1.5f, 0.8f },
                                              {2.0f, 0.5f }, {2.5f, 0.5f }, {3.0f, 0.4f }, {3.5f, 0.0f } });


// Helper - tick mood manager for N ticks with a given timestep
void TickMoodManager(MoodManager& moodManager, uint32_t numTicks, float tickTimeStep)
{
  for (uint32_t i=0; i < numTicks; ++i)
  {
    gCurrentTime += tickTimeStep;
    moodManager.Update(gCurrentTime);
    BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  }
}


void InitStaticMoodData()
{
  StaticMoodData& staticMoodData = MoodManager::GetStaticMoodData();
 
  staticMoodData.SetDecayGraph(EmotionType::Happy, kTestDecayGraph);
  staticMoodData.SetDefaultRepetitionPenalty(kTestDefaultRepetitionPenaltyGraph);
}


TEST(MoodManager, AddEmotionNoPenalty)
{
  InitStaticMoodData();
  
  MoodManager moodManager;

  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 1),   0.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1a", gCurrentTime);
  moodManager.AddToEmotion(EmotionType::Calm,  0.1f, "Test1b", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 1),   0.1f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test2", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  1.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test3", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  1.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test4", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -0.5f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1), -0.5f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test5", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1), -1.0f, 0.0f);
}


TEST(MoodManager, AddEmotionRepeatPenalty)
{
  InitStaticMoodData();
  
  MoodManager moodManager;
  
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 1),   0.0f, 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1",  gCurrentTime);
  moodManager.AddToEmotion(EmotionType::Calm,  0.1f, "Test1b", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 1),   0.1f, 0.0f);
  
  // Repeat at t=0
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1",  gCurrentTime); // 0 second since last Test1 -> only +0
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f);
  
  // Repeat at t=5
  gCurrentTime += 5.0f;
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1",  gCurrentTime); // 5 second since last Test1 -> only +0.25
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.75f);

  // Repeat at t=15
  gCurrentTime += 10.0f;
  moodManager.AddToEmotion(EmotionType::Happy, 0.24f, "Test1",  gCurrentTime); // 10 second since last Test1 -> full 0.24
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.99f);
}


TEST(MoodManager, AddEmotionNoPenaltyEachTick)
{
  InitStaticMoodData();
  
  MoodManager moodManager;
  
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy,  1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Happy, 99),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm,  1),   0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(EmotionType::Calm, 99),   0.0f, 0.0f);
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test1", gCurrentTime);
  moodManager.AddToEmotion(EmotionType::Calm,  0.1f, "Test2", gCurrentTime);
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
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test3", gCurrentTime);
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
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.5f, "Test4", gCurrentTime);
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
  
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test5", gCurrentTime);
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
  
  moodManager.AddToEmotion(EmotionType::Happy, -1.5f, "Test6", gCurrentTime);
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
  InitStaticMoodData();
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 10.0f, "Test1", gCurrentTime);
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
  InitStaticMoodData();
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, -10.0f, "Test1", gCurrentTime);
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
  InitStaticMoodData();
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 0.0f);
  moodManager.AddToEmotion(EmotionType::Happy, 10.0f, "Test1", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), 1.0f);
  
  TickMoodManager(moodManager, 20, 0.1f); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.5f,  0.000001f);

  moodManager.AddToEmotion(EmotionType::Happy, 0.25f, "Test2", gCurrentTime);
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.75f, 0.000001f);
  // now = 0.0f (add reset the decay)
  
  TickMoodManager(moodManager, 20, 0.1f); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.375f,  0.1f);

  moodManager.AddToEmotion(EmotionType::Happy, -0.25f, "Test3", gCurrentTime);
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.125f, 0.000001f);
  // now still = 2.0f (subtract doesn't reset the decay)
  // Note: due to the decrease this is equivalent to a 0.25 initial value dropped to 0.125 after 2 seconds

  TickMoodManager(moodManager, 10, 0.1f); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(EmotionType::Happy), 0.1f,  0.0001f);
  
}


static const char* kTestBehavior1Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"TestHappy\","
"   \"repetitionPenalty\" :"
"   {"
"     \"nodes\" :"
"     ["
"       {"
"         \"x\" : 1.0,"
"         \"y\" : 0.0"
"       },"
"       {"
"         \"x\" : 5.0,"
"         \"y\" : 1.0"
"       }"
"     ]"
"   }"
"}";


static const char* kTestBehavior2Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"TestCalm\","
"   \"repetitionPenalty\" :"
"   {"
"     \"nodes\" :"
"     ["
"       {"
"         \"x\" : 2.0,"
"         \"y\" : 0.0"
"       },"
"       {"
"         \"x\" : 4.0,"
"         \"y\" : 1.0"
"       }"
"     ]"
"   }"
"}";


TEST(MoodManager, BehaviorScoring)
{
  InitStaticMoodData();
  
  CozmoContext context{};
  Robot testRobot(0, &context);
  
  BehaviorFactory& behaviorFactory = testRobot.GetBehaviorFactory();

  MoodManager& moodManager = testRobot.GetMoodManager();
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  Json::Value  testBehavior1Json;
  Json::Value  testBehavior2Json;
  Json::Reader reader;
  bool parsedOK = reader.parse(kTestBehavior1Json, testBehavior1Json, false);
  ASSERT_TRUE(parsedOK);
  parsedOK = reader.parse(kTestBehavior2Json, testBehavior2Json, false);
  ASSERT_TRUE(parsedOK);
  
  // have to alloc the behaviors - they're freed by the chooser
  IBehavior* testBehaviorReqHappy = behaviorFactory.CreateBehavior(testBehavior1Json, testRobot);
  IBehavior* testBehaviorReqCalm  = behaviorFactory.CreateBehavior(testBehavior2Json, testRobot);
  ASSERT_NE(testBehaviorReqHappy, nullptr);
  ASSERT_NE(testBehaviorReqCalm,  nullptr);
  
  Json::Value chooserConfig;
  SimpleBehaviorChooser behaviorChooser(testRobot, chooserConfig);
  
  behaviorChooser.TryAddBehavior(testBehaviorReqHappy);
  behaviorChooser.TryAddBehavior(testBehaviorReqCalm);
  
  testBehaviorReqHappy->ClearEmotionScorers();
  testBehaviorReqHappy->AddEmotionScorer(EmotionScorer(EmotionType::Happy, Anki::Util::GraphEvaluator2d({{-1.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 0.6f}}), false));
  testBehaviorReqCalm->ClearEmotionScorers();
  testBehaviorReqCalm->AddEmotionScorer( EmotionScorer(EmotionType::Calm,  Anki::Util::GraphEvaluator2d({{-1.0f, 0.5f}, {0.5f, 0.0f}, {1.0f, 0.0f}}), false));

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  
  float score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  float score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  
  EXPECT_FLOAT_EQ(score1, 0.6666666666f);
  EXPECT_FLOAT_EQ(score2, 0.16666666f);
  
  {
    IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, nullptr);
    EXPECT_EQ(behaviorChosen, testBehaviorReqHappy);
  }
  
  moodManager.AddToEmotion(EmotionType::Happy, 0.25f, "Test1", gCurrentTime);
  moodManager.AddToEmotion(EmotionType::Calm,  0.5f, "Test2", gCurrentTime);
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  
  EXPECT_FLOAT_EQ(score1, 0.83333331f);
  EXPECT_FLOAT_EQ(score2, 0.0f);
  
  {
    IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, nullptr);
    EXPECT_EQ(behaviorChosen, testBehaviorReqHappy);
  }
  
  moodManager.AddToEmotion(EmotionType::Happy, -2.0f, "Test3", gCurrentTime);
  moodManager.AddToEmotion(EmotionType::Calm,  -2.0f, "Test4", gCurrentTime);
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  
  EXPECT_FLOAT_EQ(score1, 0.0f);
  EXPECT_FLOAT_EQ(score2, 0.5f);
  
  {
    IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, nullptr);
    EXPECT_EQ(behaviorChosen, testBehaviorReqCalm);
  }

  moodManager.AddToEmotion(EmotionType::Happy, 0.75f, "Test5", gCurrentTime);
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  
  EXPECT_FLOAT_EQ(score1, 0.5f);
  EXPECT_FLOAT_EQ(score2, 0.5f);
  
  // Equal scores - check that random factor will enable either behavior to be chosen
  {
    uint32_t behaviorCountHappy = 0;
    uint32_t behaviorCountCalm  = 0;
    const uint32_t kNumTests = 1000;

    for (uint32_t i=0; i < kNumTests; ++i)
    {
      IBehavior* behaviorChosen = behaviorChooser.ChooseNextBehavior(testRobot, nullptr);
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
  
  // Test behavior repetion penalty (happy 1,0->5,1, calm 2,0->4,1)

  // 1) never happened:
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.5f, 1e-4);
  EXPECT_NEAR(score2, 0.5f, 1e-4);
  
  // 2) happy happened 0.0 seconds ago:
  
  testBehaviorReqHappy->Stop();

  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.0f, 1e-4);
  EXPECT_NEAR(score2, 0.5f, 1e-4);

  // 3) happy happened 1.0 seconds ago, calm 0.0 seconds agos
  
  gCurrentTime += 1.0;

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  testBehaviorReqCalm->Stop();
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.0f, 1e-4);
  EXPECT_NEAR(score2, 0.0f, 1e-4);
  
  // 4) happy happened 2.0 seconds ago, calm 1.0 seconds agos
  
  gCurrentTime += 1.0;
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.25f * 0.5f, 1e-4);
  EXPECT_NEAR(score2, 0.0f, 1e-4);

  // 5) happy happened 3.0 seconds ago, calm 2.0 seconds agos
  
  gCurrentTime += 1.0;
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.5f * 0.5f, 1e-4);
  EXPECT_NEAR(score2, 0.0f, 1e-4);

  // 5) happy happened 4.0 seconds ago, calm 3.0 seconds agos
  
  gCurrentTime += 1.0;
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.75f * 0.5f, 1e-4);
  EXPECT_NEAR(score2, 0.5f * 0.5f, 1e-4);

  // 5) happy happened 5.0 seconds ago, calm 4.0 seconds agos
  
  gCurrentTime += 1.0;
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
  
  score1 = testBehaviorReqHappy->EvaluateScore(testRobot);
  score2 = testBehaviorReqCalm->EvaluateScore(testRobot);
  EXPECT_NEAR(score1, 0.5f, 1e-4);
  EXPECT_NEAR(score2, 0.5f, 1e-4);
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
    newEvent->SetName("TestEvent1b");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Brave, 0.11f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy, 0.22f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Calm,  0.33f));

    testEventMapper.AddEvent(newEvent);
  }
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent2b");
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Calm,  0.44f));
    newEvent->AddEmotionAffector(EmotionAffector(EmotionType::Happy, 0.55f));
    
    testEventMapper.AddEvent(newEvent);
  }
  
  {
    // This event will clobber 1st event added
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent1b");
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
    const EmotionEvent* foundEvent2 = testEventMapper2.FindEvent("TestEvent2b");
    ASSERT_NE(foundEvent2, nullptr);

    EXPECT_EQ(foundEvent2->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetValue(), 0.44f);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetValue(), 0.55f);
    
    const EmotionEvent* foundEvent1 = testEventMapper2.FindEvent("TestEvent1b");
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
"         \"name\" : \"TestEvent1c\""
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
"         \"name\" : \"TestEvent2c\""
"      }"
"   ]"
"}";

// Valid Json
static const char* kTestEmotionEventMapperJson2 =
"{"
"   \"emotionEvents\" : ["
"      {"
"         \"emotionAffectors\" : ["
"            {"
"               \"emotionType\" : \"Happy\","
"               \"value\" : 0.55"
"            }"
"         ],"
"         \"name\" : \"TestEvent1c2\""
"      },"
"      {"
"         \"emotionAffectors\" : ["
"            {"
"               \"emotionType\" : \"Brave\","
"               \"value\" : 0.66"
"            }"
"         ],"
"         \"name\" : \"TestEvent2c2\""
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
  
  // Load an additional loose event
  {
    Json::Value  emotionEventJson;
    Json::Reader reader;
    const bool parsedOK = reader.parse(kTestEmotionEventJson, emotionEventJson, false);
    ASSERT_TRUE(parsedOK);
    
    const bool readLooseRes = testEventMapper.LoadEmotionEvents(emotionEventJson);
    EXPECT_TRUE(readLooseRes);
  }
  
  // Load an additional set of loose events
  {
    Json::Value  emotionEventsJson;
    Json::Reader reader;
    const bool parsedOK = reader.parse(kTestEmotionEventMapperJson2, emotionEventsJson, false);
    ASSERT_TRUE(parsedOK);
    
    const bool readLooseRes = testEventMapper.LoadEmotionEvents(emotionEventsJson);
    EXPECT_TRUE(readLooseRes);
  }

  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent1c");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.11f);
    EXPECT_EQ(foundEvent->GetAffector(1).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent->GetAffector(1).GetValue(), 0.22f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent2c");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  EmotionType::Brave);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.33f);
    EXPECT_EQ(foundEvent->GetAffector(1).GetType(),  EmotionType::Confident);
    EXPECT_EQ(foundEvent->GetAffector(1).GetValue(), 0.44f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent1");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 3);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  EmotionType::Brave);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.11f);
    EXPECT_EQ(foundEvent->GetAffector(1).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent->GetAffector(1).GetValue(), 0.22f);
    EXPECT_EQ(foundEvent->GetAffector(2).GetType(),  EmotionType::Calm);
    EXPECT_EQ(foundEvent->GetAffector(2).GetValue(), 0.33f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent1c2");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 1);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  EmotionType::Happy);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.55f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent2c2");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 1);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  EmotionType::Brave);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.66f);
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
  testStaticData.SetDefaultRepetitionPenalty(kTestDefaultRepetitionPenaltyGraph);
  
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
"    },"
"   \"defaultRepetitionPenalty\" : {"
"      \"nodes\" : ["
"            {"
"               \"x\" : 0.0,"
"               \"y\" : 0.0"
"            },"
"            {"
"               \"x\" : 30.0,"
"               \"y\" : 1.0"
"            }"
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
  moodManager.TriggerEmotionEvent("TestEvent1", 0.0);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Brave),  0.11f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Happy), -0.22f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),   0.33f);
  moodManager.TriggerEmotionEvent("TestEvent2", 0.0);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Brave),  0.11f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Happy),  0.33f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(EmotionType::Calm),  -0.11f);
}


