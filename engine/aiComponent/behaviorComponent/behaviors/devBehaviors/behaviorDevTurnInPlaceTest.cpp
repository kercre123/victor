/**
 * File: BehaviorDevTurnInPlaceTest.cpp
 *
 * Author: Matt Michini
 * Date:   05/22/2017
 *
 * Description: Simple test for TurnInPlace action at various speeds/accels, etc.
 *              Test configuration defined in the behavior's json file.
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevTurnInPlaceTest.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace{
  const char* kChannelName = "Behaviors";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTurnInPlaceTest::BehaviorDevTurnInPlaceTest(const Json::Value& config)
: ICozmoBehavior(config)
{
  using namespace JsonTools;
  const char* debugName = "BehaviorDevTurnInPlaceTest";
  
  _loopForever = ParseBool(config, "loopForever", debugName);
  _gapBetweenTests_s = ParseFloat(config, "gapBetweenTests_s", debugName);
  _nRunsPerTest = ParseUint8(config, "nRunsPerTest", debugName);
  
  const auto& testArray = config["tests"];
  for (const auto& test : testArray) {
    const auto angle = ParseFloat(test, "angle_deg", debugName);
    const auto speed = ParseFloat(test, "speed_deg_per_sec", debugName);
    const auto accel = ParseFloat(test, "accel_deg_per_sec2", debugName);
    const auto tol   = ParseFloat(test, "tol_deg",   debugName);
    
    _tests.emplace_back(angle, speed, accel, tol);
  }
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.LoadFromJson",
                "Loaded %zu tests from config file.",
                _tests.size());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevTurnInPlaceTest::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return (ANKI_DEV_CHEATS != 0);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  Reset();
  
  DEV_ASSERT(!_tests.empty(), "BehaviorDevTurnInPlaceTest.NoTestsLoaded");
  DEV_ASSERT(_testInd == 0, "BehaviorDevTurnInPlaceTest.TestIndexNonzero");
  DEV_ASSERT(_nRunsPerTest > 0, "BehaviorDevTurnInPlaceTest.InvalidNRunsPerTest");
  
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  // Start the tests rolling
  const auto action = GenerateTestAction(robotInfo.GetPose(), _testInd);
  DelegateIfInControl(action, &BehaviorDevTurnInPlaceTest::ActionCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::Reset()
{
  _testInd = 0;
  for (auto& test : _tests) {
    test.Reset();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::ActionCallback(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  auto& currTest = _tests[_testInd];
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.TestComplete",
                "index %i, run %d, finalHeading_deg %.3f",
                _testInd, 1 + currTest.nTimesRun,
                robotInfo.GetPose().GetRotation().GetAngleAroundZaxis().getDegrees());
  
  // check if we should run this test again or move on to the next one
  if ((++currTest.nTimesRun % _nRunsPerTest) == 0) {
    ++_testInd;
  }
  
  // check if we've reached the end of the test queue
  if (_testInd >= _tests.size()) {
    if (_loopForever) {
      _testInd = 0;
    } else {
      return;
    }
  }
  
  const auto action = GenerateTestAction(robotInfo.GetPose(), _testInd);
  DelegateIfInControl(action, &BehaviorDevTurnInPlaceTest::ActionCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompoundActionSequential* BehaviorDevTurnInPlaceTest::GenerateTestAction(const Pose3d& robotPose, const int testInd) const
{
  DEV_ASSERT(testInd < _tests.size(), "BehaviorDevTurnInPlaceTest.GenerateTestAction.TestIndexOOB");
  
  const auto test = _tests[testInd];
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.TestStart",
                "index %i, run %d, angle_deg %.3f, speed_deg_per_sec %.3f, accel_deg_per_sec2 %.3f, tol_deg %.3f, startingHeading_deg %.3f",
                testInd, 1 + test.nTimesRun,
                test.angle_deg, test.speed_deg_per_sec, test.accel_deg_per_sec2, test.tol_deg,
                robotPose.GetRotation().GetAngleAroundZaxis().getDegrees());
  
  auto turnAction = new TurnInPlaceAction(DEG_TO_RAD(test.angle_deg), false);
  turnAction->SetAccel(DEG_TO_RAD(test.accel_deg_per_sec2));
  turnAction->SetMaxSpeed(DEG_TO_RAD(test.speed_deg_per_sec));
  turnAction->SetTolerance(DEG_TO_RAD(test.tol_deg));
  turnAction->SetVariability(0.f);
  
  // Optionally wait in between TurnInPlace actions
  const auto waitAction = new WaitAction(_gapBetweenTests_s);
  
  // Create the compound action
  auto compoundAction = new CompoundActionSequential();
  compoundAction->AddAction(turnAction);
  compoundAction->AddAction(waitAction);
  
  return compoundAction;
}


}
}
