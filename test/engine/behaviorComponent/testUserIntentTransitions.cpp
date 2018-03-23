/**
 * File: testUserIntentsTransitions.cpp
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
#include "test/engine/behaviorComponent/testIntentsFramework.h"
#include "test/engine/behaviorComponent/testRegistrar.h"

using namespace Anki;
using namespace Anki::Cozmo;

namespace {

MAKE_REGISTRAR( UserIntentsRegistrar )
#define TEST_INTENT(a,b, testID) static UserIntentsRegistrar autoRegUserIntents_##b(testID);\
        TEST(a,b)
  
TEST_INTENT(UserIntentsTransitions, SetTimer, "set_timer")
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

  TestIntentsFramework tif;
  auto res = tif.TestUserIntentTransition(tbf, stack, timerIntent, BehaviorID::SingletonTimerSet);
  EXPECT_TRUE(res);
}

TEST(UserIntentsTransitions, CompletedHaveTests)
{
  TestIntentsFramework tif;
  const auto& completedLabels = tif.GetCompletedLabels( TestIntentsFramework::EHandledLocation::Elsewhere );
  for( const auto& label : completedLabels ) {
    EXPECT_TRUE( UserIntentsRegistrar::IsRegistered( label ) ) << "You need to write a test for " << label;
  }
}

} // namespace
