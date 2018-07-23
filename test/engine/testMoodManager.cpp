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

// Access protected factory functions for test purposes
#define protected public

#include "gtest/gtest.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/emotionAffector.h"
#include "engine/moodSystem/emotionEvent.h"
#include "engine/moodSystem/emotionEventMapper.h"
#include "engine/moodSystem/emotionScorer.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/moodSystem/staticMoodData.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <assert.h>


using namespace Anki;
using namespace Anki::Cozmo;


const double kTickTimestep = 0.01;
double gCurrentTime = 0.0; // Fake time for update calls

Anki::Util::GraphEvaluator2d kTestDefaultRepetitionPenaltyGraph({{0.0f, 0.0}, {10.0f, 1.0f}}); // no-repetition penalty
  
Anki::Util::GraphEvaluator2d kTestGraphTimeRatio({{0.0f, 1.0f }, {0.5f, 1.0f }, {1.0f, 0.9f }, {1.5f, 0.8f },
                                                  {2.0f, 0.5f }, {2.5f, 0.5f }, {3.0f, 0.4f }, {3.5f, 0.0f } });

Anki::Util::GraphEvaluator2d kTestGraphValueSlope({ {-1.0f, 60.0f}, {-0.8f, 6.0f}, {-0.2f, 6.0f}, {0.0f, 0.6f},
                                                    {0.93999f, 0.6f}, {0.93999f, 0.0f} });


const EmotionType kTestEmoType0 = EmotionType::Happy;
const EmotionType kTestEmoType1 = EmotionType::Confident;
const EmotionType kTestEmoType2 = EmotionType::Social;
const EmotionType kTestEmoType3 = EmotionType::Stimulated;

using DGT = MoodDecayEvaulator::DecayGraphType;

// Helper - tick mood manager for N ticks with a given timestep
void TickMoodManager(MoodManager& moodManager, uint32_t numTicks, float tickTimeStep)
{
  DependencyManagedEntity<RobotComponentID> dependencies;
  for (uint32_t i=0; i < numTicks; ++i)
  {
    gCurrentTime += tickTimeStep;
    BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
    moodManager.UpdateDependent(dependencies);
  }
}


void InitStaticMoodData(const MoodDecayEvaulator::DecayGraphType graphType)
{
  StaticMoodData& staticMoodData = MoodManager::GetStaticMoodData();

  switch(graphType)
  {
    case DGT::TimeRatio:
      staticMoodData.SetDecayEvaluator(kTestEmoType0, kTestGraphTimeRatio, DGT::TimeRatio);
      break;
    case DGT::ValueSlope:
      staticMoodData.SetDecayEvaluator(kTestEmoType0, kTestGraphValueSlope, DGT::ValueSlope);
      break;
  }
  
  staticMoodData.SetDefaultRepetitionPenalty(kTestDefaultRepetitionPenaltyGraph);
}


TEST(MoodManager, AddEmotionNoPenalty)
{
  InitStaticMoodData(DGT::TimeRatio);
  
  MoodManager moodManager;

  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 1),   0.0f, 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test1a", gCurrentTime);
  moodManager.AddToEmotion(kTestEmoType1,  0.1f, "Test1b", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 1),   0.1f, 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test2", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1),  1.0f, 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test3", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1),  1.0f, 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, -1.5f, "Test4", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -0.5f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1), -0.5f, 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, -1.5f, "Test5", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1), -1.0f, 0.0f);
}


TEST(MoodManager, AddEmotionRepeatPenalty)
{
  InitStaticMoodData(DGT::TimeRatio);
  
  MoodManager moodManager;
  
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 1),   0.0f, 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test1",  gCurrentTime);
  moodManager.AddToEmotion(kTestEmoType1,  0.1f, "Test1b", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 1),   0.1f, 0.0f);
  
  // Repeat at t=0
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test1",  gCurrentTime); // 0 second since last Test1 -> only +0
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.5f);
  
  // Repeat at t=5
  gCurrentTime += 5.0f;
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test1",  gCurrentTime); // 5 second since last Test1 -> only +0.25
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.75f);

  // Repeat at t=15
  gCurrentTime += 10.0f;
  moodManager.AddToEmotion(kTestEmoType0, 0.24f, "Test1",  gCurrentTime); // 10 second since last Test1 -> full 0.24
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.99f);
}


TEST(MoodManager, AddEmotionNoPenaltyEachTick)
{
  InitStaticMoodData(DGT::TimeRatio);

  MoodManager moodManager;
  
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 99),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1,  1),   0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 99),   0.0f, 0.0f);
  
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test1", gCurrentTime);
  moodManager.AddToEmotion(kTestEmoType1,  0.1f, "Test2", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 99),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1,  1),   0.1f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 99),   0.1f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.5f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.1f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  0),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 98),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1,  0),   0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1,  1),   0.1f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType1, 98),   0.1f, 0.0f);
  
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test3", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  0),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  1),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 98),  1.0f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  2),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 99),  1.0f, 0.0f);
  
  moodManager.AddToEmotion(kTestEmoType0, 0.5f, "Test4", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   0),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   1),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   2),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  98),  1.0f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   1),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   2),  0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   3),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  99),  1.0f, 0.0f);
  
  moodManager.AddToEmotion(kTestEmoType0, -1.5f, "Test5", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -0.5f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   0),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   1), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   2), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   3), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  98), -0.5f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   1), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   2), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   3), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   4), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  99), -0.5f, 0.0f);
  
  moodManager.AddToEmotion(kTestEmoType0, -1.5f, "Test6", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   0),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   1), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   2), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   3), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   4), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  98), -1.0f, 0.0f);
  TickMoodManager(moodManager, 1, kTickTimestep);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -1.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   1), -0.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   2), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   3), -2.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   4), -1.5f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,   5), -1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  98), -1.0f, 0.0f);
}

TEST(MoodManager, DecayFromPositive)
{
  InitStaticMoodData(DGT::TimeRatio);
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, 10.0f, "Test1", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.1f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 1.0f,  0.000001f);

  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 10), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  9), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  1), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  0), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.2f   ), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.091f ), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.09f ),  0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.0f   ), 0.0f, 0.0f);
  
  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.4f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 1.0f,  0.000001f);
  
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 41), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 40), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 39), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.391f ),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.39f   ), 0.0f, 0.0f);
  
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 1.0f,  0.000001f);
  
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 50), 1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 49), 0.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.491f ),  1.0f, 0.0f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.49f   ), 0.0f, 0.0f);

  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.8f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.94f, 0.000001f);
  
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 80),  0.94f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 79), -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 30), -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0, 15), -0.03f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentTicks(kTestEmoType0,  0),  0.00f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.791f ),   0.94f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.79f  ),  -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.3f   ),  -0.06f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.15f  ),  -0.03f, 0.0001f);
  EXPECT_NEAR(moodManager.GetEmotionDeltaRecentSeconds(kTestEmoType0, 0.0f   ),   0.0f , 0.0001f);

  TickMoodManager(moodManager, 20, kTickTimestep); // now = 1.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.9f,  0.000001f);
  
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.85f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.8f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.65f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.45f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.4f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.2f,  0.00001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.0f,  0.00001f);
  // Should remain zero
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 4.0f
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
}

TEST(MoodManager, DecayFromNegative)
{
  InitStaticMoodData(DGT::TimeRatio);
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, -10.0f, "Test1", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -1.0f);
  
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.1f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -1.0f,  0.000001f);
  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.4f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -1.0f,  0.000001f);
  TickMoodManager(moodManager, 10, kTickTimestep); // now = 0.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -1.0f,  0.000001f);
  TickMoodManager(moodManager, 30, kTickTimestep); // now = 0.8f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.94f, 0.000001f);
  TickMoodManager(moodManager, 20, kTickTimestep); // now = 1.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.9f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.85f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.8f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 1.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.65f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.5f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 2.75f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.45f, 0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.4f,  0.000001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.25f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), -0.2f,  0.00001f);
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 3.5f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0),  0.0f,  0.00001f);
  // Should remain zero
  TickMoodManager(moodManager, 25, kTickTimestep); // now = 4.0f
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
}


TEST(MoodManager, DecayResetFromAwards)
{
  InitStaticMoodData(DGT::TimeRatio);
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);
  
  
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  moodManager.AddToEmotion(kTestEmoType0, 10.0f, "Test1", gCurrentTime);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 1.0f);
  
  TickMoodManager(moodManager, 20, 0.1f); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.5f,  0.000001f);

  moodManager.AddToEmotion(kTestEmoType0, 0.25f, "Test2", gCurrentTime);
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.75f, 0.000001f);
  // now = 0.0f (add reset the decay)
  
  TickMoodManager(moodManager, 20, 0.1f); // now = 2.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.375f,  0.1f);

  moodManager.AddToEmotion(kTestEmoType0, -0.25f, "Test3", gCurrentTime);
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.125f, 0.000001f);
  // now still = 2.0f (subtract doesn't reset the decay)
  // Note: due to the decrease this is equivalent to a 0.25 initial value dropped to 0.125 after 2 seconds

  TickMoodManager(moodManager, 10, 0.1f); // now = 3.0f
  EXPECT_NEAR(moodManager.GetEmotionValue(kTestEmoType0), 0.1f,  0.0001f);
  
}

// ================================================================================
// EmotionScorer
// ================================================================================


TEST(MoodManager, EmotionScorerRoundTripJson)
{
  EmotionScorer testScorer(kTestEmoType0,
                           Anki::Util::GraphEvaluator2d({{-1.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 0.6f}}), true);
  
  EXPECT_EQ(testScorer.GetEmotionType(), kTestEmoType0);
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
  
  EXPECT_EQ(testScorer2.GetEmotionType(), kTestEmoType0);
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
"   \"emotionType\" : \"Confident\","
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

  EXPECT_EQ(testScorer.GetEmotionType(), kTestEmoType1);
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
"   \"emotionType\" : \"Confident\","
"   \"trackDelta\" : false"
"}";


TEST(MoodManager, EmotionScorerReadBadJson3)
{
  Json::Value  emotionScorerJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBad3TestEmotionScorerJson, emotionScorerJson, false);
  ASSERT_TRUE(parsedOK);
  
  EmotionScorer testScorer(kTestEmoType2, Anki::Util::GraphEvaluator2d(), false);
  const bool readRes = testScorer.ReadFromJson(emotionScorerJson);
  
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testScorer.GetEmotionType(), kTestEmoType1);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 0);
  EXPECT_EQ(testScorer.TrackDeltaScore(), false);
}


// Missing scoreGraph entry
static const char* kBad4TestEmotionScorerJson =
"{"
"   \"emotionType\" : \"Confident\","
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
  
  EmotionScorer testScorer(kTestEmoType2, Anki::Util::GraphEvaluator2d(), false);
  const bool readRes = testScorer.ReadFromJson(emotionScorerJson);
  
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testScorer.GetEmotionType(), kTestEmoType1);
  EXPECT_EQ(testScorer.GetScoreGraph().GetNumNodes(), 1);
  EXPECT_EQ(testScorer.TrackDeltaScore(), false);
}


// ================================================================================
// EmotionAffector
// ================================================================================


TEST(MoodManager, EmotionAffectorRoundTripJson)
{
  EmotionAffector testAffector(kTestEmoType0, 0.75f);
  
  Json::Value testJson;
  const bool writeRes = testAffector.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  EmotionAffector testAffector2;
  const bool readRes = testAffector2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);
  EXPECT_EQ(testAffector2.GetType(),  kTestEmoType0);
  EXPECT_EQ(testAffector2.GetValue(), 0.75f);
}


// Valid Json
static const char* kTestEmotionAffectorJson =
"{"
"   \"emotionType\" : \"Confident\","
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
  EXPECT_EQ(testAffector.GetType(),  kTestEmoType1);
  EXPECT_EQ(testAffector.GetValue(), 0.33f);
}


// ================================================================================
// EmotionEvent
// ================================================================================


TEST(MoodManager, EmotionEventRoundTripJson)
{
  EmotionEvent testEvent;
  testEvent.SetName("TestEvent1");
  testEvent.AddEmotionAffector(EmotionAffector(kTestEmoType2, 0.11f));
  testEvent.AddEmotionAffector(EmotionAffector(kTestEmoType0, 0.22f));
  testEvent.AddEmotionAffector(EmotionAffector(kTestEmoType1,  0.33f));
  
  Json::Value testJson;
  const bool writeRes = testEvent.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  EmotionEvent testEvent2;
  const bool readRes = testEvent2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);
  
  EXPECT_EQ(testEvent2.GetName(), "TestEvent1");
  EXPECT_EQ(testEvent2.GetNumAffectors(), 3);
  EXPECT_EQ(testEvent2.GetAffector(0).GetType(),  kTestEmoType2);
  EXPECT_EQ(testEvent2.GetAffector(0).GetValue(), 0.11f);
  EXPECT_EQ(testEvent2.GetAffector(1).GetType(),  kTestEmoType0);
  EXPECT_EQ(testEvent2.GetAffector(1).GetValue(), 0.22f);
  EXPECT_EQ(testEvent2.GetAffector(2).GetType(),  kTestEmoType1);
  EXPECT_EQ(testEvent2.GetAffector(2).GetValue(), 0.33f);
}


// Valid Json
static const char* kTestEmotionEventJson =
"{"
"   \"name\" : \"TestEvent1\","
"   \"emotionAffectors\" : ["
"      {"
"         \"emotionType\" : \"Social\","
"         \"value\" : 0.11"
"      },"
"      {"
"         \"emotionType\" : \"Happy\","
"         \"value\" : 0.22"
"      },"
"      {"
"         \"emotionType\" : \"Confident\","
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
  EXPECT_EQ(testEvent.GetAffector(0).GetType(),  kTestEmoType2);
  EXPECT_EQ(testEvent.GetAffector(0).GetValue(), 0.11f);
  EXPECT_EQ(testEvent.GetAffector(1).GetType(),  kTestEmoType0);
  EXPECT_EQ(testEvent.GetAffector(1).GetValue(), 0.22f);
  EXPECT_EQ(testEvent.GetAffector(2).GetType(),  kTestEmoType1);
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
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType2, 0.11f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType0, 0.22f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType1,  0.33f));

    testEventMapper.AddEvent(newEvent);
  }
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent2b");
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType1,  0.44f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType0, 0.55f));
    
    testEventMapper.AddEvent(newEvent);
  }
  
  {
    // This event will clobber 1st event added
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent1b");
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType2, 0.66f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType0, 0.77f));
    
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
    EXPECT_EQ(foundEvent2->GetAffector(0).GetType(),  kTestEmoType1);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetValue(), 0.44f);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetType(),  kTestEmoType0);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetValue(), 0.55f);
    
    const EmotionEvent* foundEvent1 = testEventMapper2.FindEvent("TestEvent1b");
    ASSERT_NE(foundEvent1, nullptr);
    
    EXPECT_EQ(foundEvent1->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetType(),  kTestEmoType2);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetValue(), 0.66f);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetType(),  kTestEmoType0);
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
"               \"emotionType\" : \"Confident\","
"               \"value\" : 0.22"
"            }"
"         ],"
"         \"name\" : \"TestEvent1c\""
"      },"
"      {"
"         \"emotionAffectors\" : ["
"            {"
"               \"emotionType\" : \"Social\","
"               \"value\" : 0.33"
"            },"
"            {"
"               \"emotionType\" : \"Stimulated\","
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
"               \"emotionType\" : \"Social\","
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
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  kTestEmoType0);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.11f);
    EXPECT_EQ(foundEvent->GetAffector(1).GetType(),  kTestEmoType1);
    EXPECT_EQ(foundEvent->GetAffector(1).GetValue(), 0.22f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent2c");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  kTestEmoType2);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.33f);
    EXPECT_EQ(foundEvent->GetAffector(1).GetType(),  kTestEmoType3);
    EXPECT_EQ(foundEvent->GetAffector(1).GetValue(), 0.44f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent1");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 3);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  kTestEmoType2);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.11f);
    EXPECT_EQ(foundEvent->GetAffector(1).GetType(),  kTestEmoType0);
    EXPECT_EQ(foundEvent->GetAffector(1).GetValue(), 0.22f);
    EXPECT_EQ(foundEvent->GetAffector(2).GetType(),  kTestEmoType1);
    EXPECT_EQ(foundEvent->GetAffector(2).GetValue(), 0.33f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent1c2");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 1);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  kTestEmoType0);
    EXPECT_EQ(foundEvent->GetAffector(0).GetValue(), 0.55f);
  }
  {
    const EmotionEvent* foundEvent = testEventMapper.FindEvent("TestEvent2c2");
    ASSERT_NE(foundEvent, nullptr);
    
    EXPECT_EQ(foundEvent->GetNumAffectors(), 1);
    EXPECT_EQ(foundEvent->GetAffector(0).GetType(),  kTestEmoType2);
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
  Anki::Util::GraphEvaluator2d kTestLinearGraph({{-1.0f, 1.0f}, {1.0f, 1.0f}});

  testStaticData.InitDecayEvaluators();
  testStaticData.SetDecayEvaluator(kTestEmoType0, kTestGraphTimeRatio, DGT::TimeRatio);
  testStaticData.SetDecayEvaluator(kTestEmoType1, kTestNoDecayGraph, DGT::TimeRatio);
  // leave 2 at default
  testStaticData.SetDecayEvaluator(kTestEmoType3, kTestLinearGraph, DGT::ValueSlope);
  testStaticData.SetDefaultRepetitionPenalty(kTestDefaultRepetitionPenaltyGraph);
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent1");
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType2,  0.11f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType0, -0.22f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType1,   0.33f));
  
    testStaticData.GetEmotionEventMapper().AddEvent(newEvent);
  }
  
  {
    EmotionEvent* newEvent = new EmotionEvent();
    newEvent->SetName("TestEvent2");
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType1,  -0.44f));
    newEvent->AddEmotionAffector(EmotionAffector(kTestEmoType0,  0.55f));
    
    testStaticData.GetEmotionEventMapper().AddEvent(newEvent);
  }

  auto moodTestHelper = [](const StaticMoodData& smd) {
    float wasteRate = 0.0f;
    float wasteAccel = 0.0f;
    {
      const auto& eval = smd.GetDecayEvaluator(kTestEmoType0);
      ASSERT_FALSE(eval.Empty());
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.0f, 0.1f, wasteRate, wasteAccel), 1.0f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.4f, 0.1f, wasteRate, wasteAccel), 1.0f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(0.9f, 1.0f, 0.5f, wasteRate, wasteAccel), 0.8f);
    }

    {
      const auto& eval = smd.GetDecayEvaluator(kTestEmoType1);
      ASSERT_FALSE(eval.Empty());
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.0f, 0.1f, wasteRate, wasteAccel), 1.0f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.4f, 0.1f, wasteRate, wasteAccel), 1.0f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 100.0f, 0.5f, wasteRate, wasteAccel), 1.0f);
    }

    {
      const auto& eval = smd.GetDecayEvaluator(kTestEmoType2);
      ASSERT_FALSE(eval.Empty());
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.0f, 0.1f, wasteRate, wasteAccel), 1.0f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 10.0f, 0.1f, wasteRate, wasteAccel), 1.0f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 15.0f, 45.0f, wasteRate, wasteAccel), 0.9f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(0.9f, 60.0f, 90.0f, wasteRate, wasteAccel), 0.6f);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(0.6f, 150.0f, 150.0f, wasteRate, wasteAccel), 0.0f);
    }

    {
      const auto& eval = smd.GetDecayEvaluator(kTestEmoType3);
      ASSERT_FALSE(eval.Empty());
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.0f, 15.0f, wasteRate, wasteAccel), 0.75);
      EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 1000.0f, 15.0f, wasteRate, wasteAccel), 0.75);
    }
  };

  moodTestHelper(testStaticData);
  
  Json::Value testJson;
  const bool writeRes = testStaticData.WriteToJson(testJson);
  
  EXPECT_TRUE( writeRes );
  
  StaticMoodData testStaticData2;
  const bool readRes = testStaticData2.ReadFromJson(testJson);
  
  EXPECT_TRUE(readRes);

  moodTestHelper(testStaticData2);
  
  {
    const EmotionEvent* foundEvent1 = testStaticData2.GetEmotionEventMapper().FindEvent("TestEvent1");
    ASSERT_NE(foundEvent1, nullptr);
    
    EXPECT_EQ(foundEvent1->GetNumAffectors(), 3);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetType(),  kTestEmoType2);
    EXPECT_EQ(foundEvent1->GetAffector(0).GetValue(),  0.11f);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetType(),  kTestEmoType0);
    EXPECT_EQ(foundEvent1->GetAffector(1).GetValue(), -0.22f);
    EXPECT_EQ(foundEvent1->GetAffector(2).GetType(),  kTestEmoType1);
    EXPECT_EQ(foundEvent1->GetAffector(2).GetValue(),  0.33f);
    
    const EmotionEvent* foundEvent2 = testStaticData2.GetEmotionEventMapper().FindEvent("TestEvent2");
    ASSERT_NE(foundEvent2, nullptr);
    
    EXPECT_EQ(foundEvent2->GetNumAffectors(), 2);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetType(),  kTestEmoType1);
    EXPECT_EQ(foundEvent2->GetAffector(0).GetValue(), -0.44f);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetType(),  kTestEmoType0);
    EXPECT_EQ(foundEvent2->GetAffector(1).GetValue(),  0.55f);
  }
}


// Valid Json
static const char* kTestStaticMoodDataJson = R"json(
{
  "decayGraphs" : [
    {
      "emotionType" : "default",
      "graphType": "TimeRatio",
      "nodes" : [
        {
          "x" : 0,
          "y" : 1
        },
        {
          "x" : 10,
          "y" : 1
        },
        {
          "x" : 30,
          "y" : 0.9
        },
        {
          "x" : 120,
          "y" : 0
        }
      ]
    },
    {
      "emotionType": "Social",
      "graphType": "TimeRatio",
      "nodes": [
        {
          "x": 0,
          "y": 1
        },
        {
          "x": 2,
          "y": 1
        },
        {
          "x": 12,
          "y": 0.5
        },
        {
          "x": 30,
          "y": 0
        }
      ]
    },
    {
      "emotionType": "Stimulated",
      "graphType": "ValueSlope",
      "nodes": [
        {
          "x": 0,
          "y": 0.6
        },
        {
          "x": 0.2,
          "y": 0.06
        },
        {
          "x": 0.8,
          "y": 0.06
        },
        {
          "x": 0.8,
          "y": 1.2
        },
        {
          "x": 0.99,
          "y": 1.2
        },
        {
          "x": 0.99,
          "y": 0.0
        }
      ]
    }
  ],

  "valueRanges": [
    {
      "emotionType": "Stimulated",
      "min": 0.0,
      "max": 1.0
    }
  ],
  "eventMapper" : {
    "emotionEvents" : [
      {
        "emotionAffectors" : [
          {
            "emotionType" : "Social",
            "value" : 0.11
          },
          {
            "emotionType" : "Happy",
            "value" : -0.22
          },
          {
            "emotionType" : "Confident",
            "value" : 0.33
          }
        ],
        "name" : "TestEvent1"
      },
      {
        "emotionAffectors" : [
          {
            "emotionType" : "Confident",
            "value" : -0.44
          },
          {
            "emotionType" : "Happy",
            "value" : 0.55
          }
        ],
        "name" : "TestEvent2"
      }
    ]
  },
  "audioParameterMap": {
    "Happy": "Robot_Vic_Happy",
    "Confident": "Robot_Vic_Confident",
    "Social": "Robot_Vic_Social",
    "Stimulated": "Robot_Vic_Stimulation"
  },
  "defaultRepetitionPenalty" : {
    "nodes" : [
      {
        "x" : 0.0,
        "y" : 0.0
      },
      {
        "x" : 0.5,
        "y" : 0.0
      },
      {
        "x" : 10.0,
        "y" : 1.0
      }
    ]
  },
  "actionResultEmotionEvents": {
    "DRIVE_TO_POSE": {
      "RETRY": "DrivingActionFailedWithRetry",
      "ABORT": "DrivingActionFailedWithAbort"
    },
    "PICKUP_OBJECT_LOW": {
      "SUCCESS": "PickupSucceeded",
      "RETRY": "PickingOrPlacingActionFailedWithRetry",
      "ABORT": "PickingOrPlacingActionFailedWithAbort"
    }
  }
}
)json";

TEST(MoodManager, StaticMoodDataReadJson)
{
  Json::Value  staticMoodDataJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestStaticMoodDataJson, staticMoodDataJson, false);
  ASSERT_TRUE(parsedOK);
  
  StaticMoodData& testStaticMoodData = MoodManager::GetStaticMoodData();
  const bool readRes = testStaticMoodData.ReadFromJson(staticMoodDataJson);
  
  EXPECT_TRUE(readRes);

  static const float kNearTolerance = 0.001f;
  
  // only check Social and Stimulated rate and accel values (to hit distinct decay evaluators), and the rest are dummy params
  float wasteRate = 0.0f;
  float wasteAccel = 0.0f;
  
  auto compare = [](const MoodDecayEvaulator& eval, float currentVal, float timeSinceEvent, float deltaTime_s,
                    float expectedValue, float expectedRate, float expectedAccel)
  {
    float rate;
    float accel;
    EXPECT_FLOAT_EQ(eval.EvaluateDecay(currentVal, timeSinceEvent, deltaTime_s, rate, accel), expectedValue);
    EXPECT_FLOAT_EQ(expectedRate, rate);
    EXPECT_FLOAT_EQ(expectedAccel, accel);
  };
  
  {
    const auto& eval = testStaticMoodData.GetDecayEvaluator(EmotionType::Social);
    ASSERT_FALSE(eval.Empty());
    compare(eval, 1.0f,  0.0f, 0.1f,   1.0f , 0.0f, 0.0f);
    compare(eval, 1.0f,  1.9f, 0.1f,   1.0f , 0.0f, 0.0f);
    compare(eval, 0.9f,  4.0f, 1.0f,   0.85f, -0.05*1.0f, 0.0f);
    compare(eval, 1.0f, 30.0f, 0.1f,   0.0f , 0.0f, 0.0f);
    compare(eval, 0.0f, 30.0f, 0.1f,   0.0f , 0.0f, 0.0f);
  }

  {
    const auto& eval = testStaticMoodData.GetDecayEvaluator(EmotionType::Happy);
    ASSERT_FALSE(eval.Empty());
    EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 0.0f, 0.1f, wasteRate, wasteAccel), 1.0f);
    EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 9.0f, 0.1f, wasteRate, wasteAccel), 1.0f);
    EXPECT_NEAR(eval.EvaluateDecay(0.9f, 30.0f, 10.0f, wasteRate, wasteAccel), 0.8f, kNearTolerance);
    EXPECT_FLOAT_EQ(eval.EvaluateDecay(1.0f, 140.0f, 0.1f, wasteRate, wasteAccel), 0.0f);
    EXPECT_FLOAT_EQ(eval.EvaluateDecay(0.0f, 30.0f, 0.1f, wasteRate, wasteAccel), 0.0f);
  }

  {
    const auto& eval = testStaticMoodData.GetDecayEvaluator(EmotionType::Stimulated);
    ASSERT_FALSE(eval.Empty());
    compare(eval, 1.0f,  0.0f, 60.0f,   1.0f, 0.0f, 0.0f);
    compare(eval, 1.0f,  9.0f, 60.0f,   1.0f, 0.0f, 0.0f);
    compare(eval, 1.0f, 30.0f, 60.0f,   1.0f, 0.0f, 0.0f);

    compare(eval, 0.9f, 0.0f,  1.0f,   0.88f, -0.02f,  0.0f);
    compare(eval, 0.5f, 0.0f, 60.0f,   0.44f, -0.001f, 0.0f);
    compare(eval, 0.0f, 0.0f, 60.0f,    0.0f, -0.01f,  0.0f);
  }


  MoodManager moodManager;
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType2), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType0), 0.0f);
  EXPECT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  0.0f);
  moodManager.TriggerEmotionEvent("TestEvent1", 0.0);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType2),  0.11f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType0), -0.22f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType1),   0.33f);
  moodManager.TriggerEmotionEvent("TestEvent2", 0.0);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType2),  0.11f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType0),  0.33f);
  EXPECT_FLOAT_EQ(moodManager.GetEmotionValue(kTestEmoType1),  -0.11f);
}

TEST(MoodManager, SimpleMood)
{
  InitStaticMoodData(DGT::TimeRatio);
  
  MoodManager moodManager;
  TickMoodManager(moodManager, 1, kTickTimestep);

  EXPECT_EQ(moodManager.GetSimpleMood(), SimpleMoodType::LowStim);

  // contains (stim level, confident level, expected mood)
  const std::vector<std::tuple<float, float, SimpleMoodType>> expectations = {
    {1.0f, 1.0f, SimpleMoodType::HighStim},
    {1.0f, 1.0f, SimpleMoodType::HighStim},
    {1.0f, 0.5f, SimpleMoodType::HighStim},
    {1.0f, 0.0f, SimpleMoodType::HighStim},
    {1.0f, -0.1f, SimpleMoodType::HighStim},
    {1.0f, -0.5f, SimpleMoodType::Frustrated},
    {1.0f, -1.0f, SimpleMoodType::Frustrated},
    
    {0.5f, 1.0f, SimpleMoodType::MedStim},
    {0.5f, 1.0f, SimpleMoodType::MedStim},
    {0.5f, 0.5f, SimpleMoodType::MedStim},
    {0.5f, 0.0f, SimpleMoodType::MedStim},
    {0.5f, -0.1f, SimpleMoodType::MedStim},
    {0.5f, -0.5f, SimpleMoodType::Frustrated},
    {0.5f, -1.0f, SimpleMoodType::Frustrated},

    {0.0f, 1.0f, SimpleMoodType::LowStim},
    {0.0f, 1.0f, SimpleMoodType::LowStim},
    {0.0f, 0.5f, SimpleMoodType::LowStim},
    {0.0f, 0.0f, SimpleMoodType::LowStim},
    {0.0f, -0.1f, SimpleMoodType::LowStim},
    {0.0f, -0.5f, SimpleMoodType::LowStim},
    {0.0f, -1.0f, SimpleMoodType::LowStim} };


  for( const auto& tpl : expectations ) {
    moodManager.SetEmotion(EmotionType::Stimulated, std::get<0>(tpl));
    moodManager.SetEmotion(EmotionType::Confident,  std::get<1>(tpl));
    EXPECT_EQ(moodManager.GetSimpleMood(), std::get<2>(tpl))
      << "before tick: stim=" << std::get<0>(tpl) << " conf=" << std::get<1>(tpl) << " expcted "
      << SimpleMoodTypeToString(std::get<2>(tpl)) << " got " <<SimpleMoodTypeToString(moodManager.GetSimpleMood());
      
    TickMoodManager(moodManager, 1, kTickTimestep);
    EXPECT_EQ(moodManager.GetSimpleMood(), std::get<2>(tpl))
      << "after tick: stim=" << std::get<0>(tpl) << " conf=" << std::get<1>(tpl) << " expcted "
      << SimpleMoodTypeToString(std::get<2>(tpl)) << " got " <<SimpleMoodTypeToString(moodManager.GetSimpleMood());

  }  
}
