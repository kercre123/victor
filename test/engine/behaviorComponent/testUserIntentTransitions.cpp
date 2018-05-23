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

  // Build a valid stack - this test breaks if this stack is no longer valid
  std::vector<IBehavior*> stack = tbf.GetNamedBehaviorStack("driveOffChargerIntoObserving_stack");

  UserIntent_TimeInSeconds timeInSeconds(20);
  UserIntent timerIntent;
  timerIntent._tag = UserIntentTag::set_timer;
  timerIntent._set_timer = timeInSeconds;

  TestIntentsFramework tif;
  auto res = tif.TestUserIntentTransition(tbf, stack, timerIntent, BehaviorID::SingletonTimerSet);
  EXPECT_TRUE(res);
}
  
bool IntentHelper( const UserIntent& intent, BehaviorID behavior, bool onlyCheckInStack = false )
{
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  
  std::vector<IBehavior*> stack = tbf.GetNamedBehaviorStack("highLevelAI_stack");
  
  TestIntentsFramework tif;
  const bool res = tif.TestUserIntentTransition(tbf, stack, intent, behavior, onlyCheckInStack );
  return res;
}
  
bool PlaySpecificHelper( std::string entityBehavior, BehaviorID behavior, bool onlyCheckInStack = false )
{
  UserIntent_PlaySpecific playSpecificIntent(entityBehavior);
  UserIntent intent;
  intent._tag = UserIntentTag::play_specific;
  intent._play_specific = playSpecificIntent;
  
  return IntentHelper( intent, behavior, onlyCheckInStack );
}

TEST_INTENT(UserIntentsTransitions, KeepAway, "keep_away")
{
  const bool res = PlaySpecificHelper( "keep_away", BehaviorID::Keepaway );
  EXPECT_TRUE(res);
}

TEST_INTENT(UserIntentsTransitions, FistBump, "fist_bump")
{
  const bool res = PlaySpecificHelper( "fist_bump", BehaviorID::FistBumpVoiceCommand, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, RollCube, "roll_cube")
{
  const bool res = PlaySpecificHelper( "roll_cube", BehaviorID::RollCubeVoiceCommand, true );
  EXPECT_TRUE(res);
}

// todo: come here doesn't look for faces if it doesn't know about a face
//TEST_INTENT(UserIntentsTransitions, ComeHere, "imperative_come")
//{
//  UserIntent intent;
//  intent._tag = UserIntentTag::imperative_come;
//  const bool res = IntentHelper( intent, BehaviorID::ComeHereVoiceCommand, true );
//  EXPECT_TRUE(res);
//}
  
TEST_INTENT(UserIntentsTransitions, MeetVictor, "meet_victor")
{
  UserIntent intent;
  intent.Set_meet_victor( UserIntent_MeetVictor("cozmo") );
  const bool res = IntentHelper( intent, BehaviorID::MeetVictor, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, BeQuiet, "be_quiet")
{
  UserIntent intent;
  intent.Set_imperative_quiet({});
  const bool res = IntentHelper( intent, BehaviorID::BeQuietAnims, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, ShutUp, "shut_up")
{
  UserIntent intent;
  intent.Set_imperative_shutup({});
  const bool res = IntentHelper( intent, BehaviorID::ShutUpAnims, true );
  EXPECT_TRUE(res);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// the testing of the tests
TEST(UserIntentsTransitions, CompletedHaveTests)
{
  TestIntentsFramework tif;
  const auto& completedLabels = tif.GetCompletedLabels( TestIntentsFramework::EHandledLocation::Elsewhere );
  for( const auto& label : completedLabels ) {
    EXPECT_TRUE( UserIntentsRegistrar::IsRegistered( label ) ) << "You need to write a test for " << label;
  }
  for( const auto& registeredTest : UserIntentsRegistrar::RegisteredTests() ) {
    const bool extraTest = std::find( completedLabels.begin(), completedLabels.end(), registeredTest ) == completedLabels.end();
    EXPECT_FALSE( extraTest ) << "You wrote an extra test " << registeredTest << ". Typo? Or did you mean to put it elsewhere?";
  }
}

} // namespace
