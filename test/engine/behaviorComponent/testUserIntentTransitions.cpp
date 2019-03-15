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

#include "clad/types/behaviorComponent/behaviorClasses.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "test/engine/behaviorComponent/testIntentsFramework.h"
#include "test/engine/behaviorComponent/testRegistrar.h"

using namespace Anki;
using namespace Anki::Vector;

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

  UserIntent timerIntent( UserIntent_TimeInSeconds(20) ); 
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


// keep_away was removed from entity_behavior_entries_en.json in April
// (see https://github.com/anki/voice-intent-resolution-config/pull/33/files)
//
//TEST_INTENT(UserIntentsTransitions, KeepAway, "keep_away")
//{
//  const bool res = PlaySpecificHelper( "keep_away", BehaviorID::Keepaway );
//  EXPECT_TRUE(res);
//}

TEST_INTENT(UserIntentsTransitions, FistBump, "fist_bump")
{
  const bool res = IntentHelper( UserIntent::Createplay_fistbump({}), BehaviorID::FistBumpVoiceCommand, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, RollCube, "roll_cube")
{
  const bool res = IntentHelper( UserIntent::Createplay_rollcube({}), BehaviorID::RollCubeVoiceCommand, true );
  EXPECT_TRUE(res);
}

TEST_INTENT(UserIntentsTransitions, ComeHere, "imperative_come")
{
  const bool res = IntentHelper( UserIntent::Createimperative_come({}), BehaviorID::ComeHereVoiceCommand, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, LookAtMe, "imperative_lookatme")
{
  const bool res = IntentHelper( UserIntent::Createimperative_lookatme({}), BehaviorID::LookAtMeVoiceCommand, true );
  EXPECT_TRUE(res);
}

// commented out until the 'GazeDirection' feature is enabled in resources/config/features.json
// TEST_INTENT(UserIntentsTransitions, LookOverThere, "imperative_lookoverthere")
// {
//   const bool res = IntentHelper( UserIntent::Createimperative_lookoverthere({}), BehaviorID::LookOverThereVoiceCommand, true );
//   EXPECT_TRUE(res);
// }
  
TEST_INTENT(UserIntentsTransitions, WhatsMyName, "names_ask")
{
  const bool res = IntentHelper( UserIntent::Createnames_ask({}), BehaviorID::WhatsMyNameVoiceCommand, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, MeetVictor, "meet_victor")
{
  const bool res = IntentHelper( UserIntent::Createmeet_victor( UserIntent_MeetVictor("cozmo") ), BehaviorID::MeetVictor, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, BeQuiet, "be_quiet")
{
  const bool res = IntentHelper( UserIntent::Createimperative_quiet({}), BehaviorID::BeQuietAnims, true );
  EXPECT_TRUE(res);
}

TEST_INTENT(UserIntentsTransitions, GoHome, "system_charger")
{
  const bool res = IntentHelper( UserIntent::Createsystem_charger({}), BehaviorID::FindAndGoToHome, true );
  EXPECT_TRUE(res);
}
  
TEST_INTENT(UserIntentsTransitions, ShutUp, "shut_up")
{
  const bool res = IntentHelper( UserIntent::Createimperative_shutup({}), BehaviorID::ShutUpAnims, true );
  EXPECT_TRUE(res);
}

TEST_INTENT(UserIntentsTransitions, Sleep, "system_sleep")
{
  const bool res = IntentHelper( UserIntent::Createsystem_sleep({}), BehaviorID::GoToSleep, true );
  EXPECT_TRUE(res);
}

// commented out for now to avoid this problem:
// [Error] ActiveFeatureComponent.PossibleBug TELL BRAD: Feature 'VolumeAdjustment' is activating (old feature is NoFeature). No intent active, but one is pending.
// [Error] TestIntentsFramework.IntentNotConsumed ASSERT ( ++tics < kMaxTicksToClear ): Intent 'imperative_volumelevel' is still pending after the tick limit
//
// TEST_INTENT(UserIntentsTransitions, VolumeLevel, "imperative_volumelevel")
// {
//   const bool res = IntentHelper( UserIntent::Createimperative_volumelevel( UserIntent_Volume("medium") ), BehaviorID::Volume, true );
//   EXPECT_TRUE(res);
// }

// TEST_INTENT(UserIntentsTransitions, VolumeUp, "imperative_volumeup")
// {
//   const bool res = IntentHelper( UserIntent::Createimperative_volumeup({}), BehaviorID::Volume, true );
//   EXPECT_TRUE(res);
// }

// TEST_INTENT(UserIntentsTransitions, VolumeDown, "imperative_volumedown")
// {
//   const bool res = IntentHelper( UserIntent::Createimperative_volumedown({}), BehaviorID::Volume, true );
//   EXPECT_TRUE(res);
// }


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
