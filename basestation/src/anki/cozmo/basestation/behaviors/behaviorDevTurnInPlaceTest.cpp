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

#include "anki/cozmo/basestation/behaviors/behaviorDevTurnInPlaceTest.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
  const char* kChannelName = "Behaviors";
}

  
BehaviorDevTurnInPlaceTest::BehaviorDevTurnInPlaceTest(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
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
  

bool BehaviorDevTurnInPlaceTest::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return (ANKI_DEV_CHEATS != 0);
}
  

Result BehaviorDevTurnInPlaceTest::InitInternal(Robot& robot)
{
  Reset();
  
  DEV_ASSERT(!_tests.empty(), "BehaviorDevTurnInPlaceTest.NoTestsLoaded");
  DEV_ASSERT(_testInd == 0, "BehaviorDevTurnInPlaceTest.TestIndexNonzero");
  DEV_ASSERT(_nRunsPerTest > 0, "BehaviorDevTurnInPlaceTest.InvalidNRunsPerTest");
  
  // Disable reactions
  SmartDisableReactionsWithLock(GetIDStr(), ReactionTriggerHelpers::kAffectAllArray);
  
  // Start the tests rolling
  const auto action = GenerateTestAction(robot, _testInd);
  StartActing(action, &BehaviorDevTurnInPlaceTest::ActionCallback);
  
  return RESULT_OK;
}


void BehaviorDevTurnInPlaceTest::StopInternal(Robot& robot)
{

}
  
  
void BehaviorDevTurnInPlaceTest::Reset()
{
  _testInd = 0;
  for (auto& test : _tests) {
    test.Reset();
  }
}
  
  
void BehaviorDevTurnInPlaceTest::ActionCallback(Robot& robot)
{
  auto& currTest = _tests[_testInd];
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.TestComplete",
                "index %i, run %d, finalHeading_deg %.3f",
                _testInd, 1 + currTest.nTimesRun,
                robot.GetPose().GetRotation().GetAngleAroundZaxis().getDegrees());
  
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
  
  const auto action = GenerateTestAction(robot, _testInd);
  StartActing(action, &BehaviorDevTurnInPlaceTest::ActionCallback);
}
  
  
CompoundActionSequential* BehaviorDevTurnInPlaceTest::GenerateTestAction(Robot& robot, const int testInd) const
{
  DEV_ASSERT(testInd < _tests.size(), "BehaviorDevTurnInPlaceTest.GenerateTestAction.TestIndexOOB");
  
  const auto test = _tests[testInd];
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.TestStart",
                "index %i, run %d, angle_deg %.3f, speed_deg_per_sec %.3f, accel_deg_per_sec2 %.3f, tol_deg %.3f, startingHeading_deg %.3f",
                testInd, 1 + test.nTimesRun,
                test.angle_deg, test.speed_deg_per_sec, test.accel_deg_per_sec2, test.tol_deg,
                robot.GetPose().GetRotation().GetAngleAroundZaxis().getDegrees());
  
  auto turnAction = new TurnInPlaceAction(robot, DEG_TO_RAD(test.angle_deg), false);
  turnAction->SetAccel(DEG_TO_RAD(test.accel_deg_per_sec2));
  turnAction->SetMaxSpeed(DEG_TO_RAD(test.speed_deg_per_sec));
  turnAction->SetTolerance(DEG_TO_RAD(test.tol_deg));
  turnAction->SetVariability(0.f);
  
  // Optionally wait in between TurnInPlace actions
  const auto waitAction = new WaitAction(robot, _gapBetweenTests_s);
  
  // Create the compound action
  auto compoundAction = new CompoundActionSequential(robot);
  compoundAction->AddAction(turnAction);
  compoundAction->AddAction(waitAction);
  
  return compoundAction;
}


}
}
