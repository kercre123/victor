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

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {

namespace{
const char* kChannelName = "Behaviors";

const char* const kLoopForeverKey = "loopForever";
const char* const kGapBetweenTestsKey = "gapBetweenTests_s";
const char* const kRunsPerTestKey = "nRunsPerTest";
const char* const kTestsKey = "tests";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTurnInPlaceTest::InstanceConfig::InstanceConfig()
{
  gapBetweenTests_s = 0.f;
  nRunsPerTest      = 1;
  loopForever        = false;

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTurnInPlaceTest::DynamicVariables::DynamicVariables()
{
  testInd = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTurnInPlaceTest::BehaviorDevTurnInPlaceTest(const Json::Value& config)
: ICozmoBehavior(config)
{
  using namespace JsonTools;
  const char* debugName = "BehaviorDevTurnInPlaceTest";
  
  _iConfig.loopForever = ParseBool(config, kLoopForeverKey, debugName);
  _iConfig.gapBetweenTests_s = ParseFloat(config, kGapBetweenTestsKey, debugName);
  _iConfig.nRunsPerTest = ParseUint8(config, kRunsPerTestKey, debugName);
  
  const auto& testArray = config[kTestsKey];
  for (const auto& test : testArray) {
    const auto angle = ParseFloat(test, "angle_deg", debugName);
    const auto speed = ParseFloat(test, "speed_deg_per_sec", debugName);
    const auto accel = ParseFloat(test, "accel_deg_per_sec2", debugName);
    const auto tol   = ParseFloat(test, "tol_deg",   debugName);
    
    _iConfig.tests.emplace_back(angle, speed, accel, tol);
  }
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.LoadFromJson",
                "Loaded %zu tests from config file.",
                _iConfig.tests.size());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kLoopForeverKey,
    kGapBetweenTestsKey,
    kRunsPerTestKey,
    kTestsKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevTurnInPlaceTest::WantsToBeActivatedBehavior() const
{
  return (ANKI_DEV_CHEATS != 0);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::OnBehaviorActivated()
{
  Reset();
  
  DEV_ASSERT(!_iConfig.tests.empty(), "BehaviorDevTurnInPlaceTest.NoTestsLoaded");
  DEV_ASSERT(_dVars.testInd == 0, "BehaviorDevTurnInPlaceTest.TestIndexNonzero");
  DEV_ASSERT(_iConfig.nRunsPerTest > 0, "BehaviorDevTurnInPlaceTest.InvalidNRunsPerTest");
  
  auto& robotInfo = GetBEI().GetRobotInfo();
  // Start the tests rolling
  const auto action = GenerateTestAction(robotInfo.GetPose(), _dVars.testInd);
  DelegateIfInControl(action, &BehaviorDevTurnInPlaceTest::ActionCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::OnBehaviorDeactivated()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::Reset()
{
  _dVars.testInd = 0;
  for (auto& test : _iConfig.tests) {
    test.Reset();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTurnInPlaceTest::ActionCallback()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  auto& currTest = _iConfig.tests[_dVars.testInd];
  
  PRINT_CH_INFO(kChannelName,
                "BehaviorDevTurnInPlaceTest.TestComplete",
                "index %i, run %d, finalHeading_deg %.3f",
                _dVars.testInd, 1 + currTest.nTimesRun,
                robotInfo.GetPose().GetRotation().GetAngleAroundZaxis().getDegrees());
  
  // check if we should run this test again or move on to the next one
  if ((++currTest.nTimesRun % _iConfig.nRunsPerTest) == 0) {
    ++_dVars.testInd;
  }
  
  // check if we've reached the end of the test queue
  if (_dVars.testInd >= _iConfig.tests.size()) {
    if (_iConfig.loopForever) {
      _dVars.testInd = 0;
    } else {
      return;
    }
  }
  
  const auto action = GenerateTestAction(robotInfo.GetPose(), _dVars.testInd);
  DelegateIfInControl(action, &BehaviorDevTurnInPlaceTest::ActionCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompoundActionSequential* BehaviorDevTurnInPlaceTest::GenerateTestAction(const Pose3d& robotPose, const int testInd) const
{
  DEV_ASSERT(testInd < _iConfig.tests.size(), "BehaviorDevTurnInPlaceTest.GenerateTestAction.TestIndexOOB");
  
  const auto test = _iConfig.tests[testInd];
  
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
  const auto waitAction = new WaitAction(_iConfig.gapBetweenTests_s);
  
  // Create the compound action
  auto compoundAction = new CompoundActionSequential();
  compoundAction->AddAction(turnAction);
  compoundAction->AddAction(waitAction);
  
  return compoundAction;
}


}
}
