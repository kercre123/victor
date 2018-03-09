/**
 * File: testTransitionsUserIntents.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-03-07
 *
 * Description: Test behavior stack transitions as a result of user intents
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#define private public

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Cozmo;

namespace {
  
TEST(TransitionsUserIntents, SetTimer)
{
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  auto& bc = tbf.GetBehaviorContainer();

  // Build a valid stack - this test breaks if this stack is no longer valid
  std::vector<IBehavior*> stack;
  stack.push_back(bc.FindBehaviorByID(BehaviorID::DevBaseBehavior).get());
  stack.push_back(bc.FindBehaviorByID(BehaviorID::CoordinateGlobalInterrupts).get());
  stack.push_back(bc.FindBehaviorByID(BehaviorID::GlobalInterruptions).get());
  stack.push_back(bc.FindBehaviorByID(BehaviorID::HighLevelAI).get());
  stack.push_back(bc.FindBehaviorByID(BehaviorID::DriveOffChargerIntoObserving).get());
  ASSERT_TRUE(tbf.CanStackOccurDuringFreeplay(stack));

  UserIntent_TimeInSeconds timeInSeconds(20);
  UserIntent timerIntent;
  timerIntent._tag = UserIntentTag::set_timer;
  timerIntent._set_timer = timeInSeconds;

  auto res = tbf.TestUserIntentTransition(stack, timerIntent, BehaviorID::SingletonTimerSet);
  EXPECT_TRUE(res);
}


} // namespace
