/**
 * File: behaviorLiftLoadTest.cpp
 *
 * Author: Kevin Yoon
 * Date:   11/11/2016
 *
 * Description: Tests the lift load detection system
 *              Saves attempt information to log webotsCtrlGameEngine/temp
 *
 *              Init conditions:
 *                - Cozmo's lift is down. A cube is optionally placed against it.
 *
 *              Behavior:
 *                - Raise and lower lift. Optionally with a cube attached
 *                - Records the result of the lift load check
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorLiftLoadTest.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotToEngineImplMessaging.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"
#include <ctime>

#define DEBUG_LIFT_LOAD_TEST_BEHAVIOR 1

#define END_TEST_IN_HANDLER(RESULT, NAME) EndAttempt(robot, RESULT, NAME); return;

namespace{
  // This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
  // All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
if ((_BEHAVIORDEF)) { PRINT_CH_INFO("Behaviors",  __VA_ARGS__ ); } \
else { PRINT_CH_DEBUG("Behaviors",  __VA_ARGS__ ); } \
} while(0) \

}


namespace Anki {
namespace Vector {

CONSOLE_VAR(u32, kNumLiftRaises,              "LiftLoadTest", 50);

// !!! These should match the values in PickAndPlaceController for accurate testing
static const f32 DEFAULT_LIFT_SPEED_RAD_PER_SEC = 1.5;
static const f32 DEFAULT_LIFT_ACCEL_RAD_PER_SEC2 = 10;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLiftLoadTest::InstanceConfig::InstanceConfig()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLiftLoadTest::DynamicVariables::DynamicVariables()
{
  currentState       = State::Init;
  abortTest          = false;
  canRun             = true;
  numLiftRaises      = 0;
  numHadLoad         = 0;
  loadStatusReceived = false;
  hasLoad            = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLiftLoadTest::BehaviorLiftLoadTest(const Json::Value& config)
: ICozmoBehavior(config)
{
  
  SubscribeToTags({
    EngineToGameTag::RobotOffTreadsStateChanged
  });
  
  SubscribeToTags({
    GameToEngineTag::SetLiftLoadTestAsActivatable
  });
  
  SubscribeToTags({
    RobotInterface::RobotToEngineTag::liftLoad
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::InitBehavior()
{
  _iConfig.logger = std::make_unique<Util::RollingFileLogger>(nullptr, 
    GetBEI().GetRobotInfo()._robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, "liftLoadTest"));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLiftLoadTest::WantsToBeActivatedBehavior() const
{
  return _dVars.canRun && (_dVars.currentState == State::Init || _dVars.currentState == State::TestComplete);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::OnBehaviorActivated()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  _dVars.abortTest = false;
  _dVars.currentState = State::Init;
  _dVars.numLiftRaises = 0;
  _dVars.numHadLoad = 0;

  robot.GetActionList().Cancel();
  
  
  Write("=====Start LiftLoadTest=====");
  std::stringstream ss;
  ss << "RobotSN: " << robot.GetHeadSerialNumber() << "\n";
  ss << "Battery(V): " << robot.GetBatteryComponent().GetBatteryVoltsRaw() << "\n\n";
  ss << "OS Version: " << OSState::getInstance()->GetOSBuildVersion() << "\n";
  Write(ss.str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(_dVars.numLiftRaises == kNumLiftRaises || _dVars.abortTest)
  {
    if (_dVars.numLiftRaises == kNumLiftRaises) {
      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.UpdateInternal_Legacy.TestComplete", "%d", _dVars.numLiftRaises);
      Write("\nTest Completed Successfully");
    } else {
      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.UpdateInternal_Legacy.TestAborted", "%d", _dVars.numLiftRaises);
      Write("\nTest Aborted");
    }
    
    PrintStats();
    _dVars.abortTest = false;
    _dVars.numLiftRaises = 0;
    _dVars.canRun = false;
    
    SetCurrState(State::TestComplete);
    CancelSelf();
    return;
  }
  
  if(IsControlDelegated())
  {
    return;
  }
  
  switch(_dVars.currentState)
  {
    case State::Init:
    {
      Robot& robot = GetBEI().GetRobotInfo()._robot;
      auto lowerLiftAction = new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK);
      lowerLiftAction->SetMaxLiftSpeed(DEFAULT_LIFT_SPEED_RAD_PER_SEC);
      lowerLiftAction->SetLiftAccel(DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
      
      auto raiseLiftAction = new MoveLiftToHeightAction(LIFT_HEIGHT_CARRY);
      raiseLiftAction->SetMaxLiftSpeed(DEFAULT_LIFT_SPEED_RAD_PER_SEC);
      raiseLiftAction->SetLiftAccel(DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
      
      CompoundActionSequential* compoundAction = new CompoundActionSequential({ lowerLiftAction, raiseLiftAction });
      
      DelegateIfInControl(compoundAction,
                  [this, &robot](ActionResult result){
                    if (ActionResult::SUCCESS == result) {
                      // Tell robot to check for load and wait for response
                      robot.SendRobotMessage<RobotInterface::CheckLiftLoad>();
                      
                      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.WaitingForLiftLoadMsg", "");
                      _dVars.loadStatusReceived = false;
                      auto waitForLiftLoadMsgLambda = [this](Robot& robot) {
                        return _dVars.loadStatusReceived;
                      };
                      auto waitAction = new WaitForLambdaAction(waitForLiftLoadMsgLambda);
                      DelegateIfInControl(waitAction);
                      
                    } else {
                      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.LiftActionFailed","");
                      
                      // Lift action failed for some reason. Retry.
                      Write("\nLift actions failed. Retrying.");
                    }
                  });
      break;
    }
    case State::WaitForPutdown:
    {
      if(GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads){
        _dVars.abortTest = true;
      }
      break;
    }
    case State::TestComplete:
    {
      CancelSelf();
      return;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorLiftLoadTest.Update.UnknownState", "Reached unknown state %d", (u32)_dVars.currentState);
      CancelSelf();
      return;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::OnBehaviorDeactivated()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::SetCurrState(State s)
{
  _dVars.currentState = s;
  
  UpdateStateName();
  
  BEHAVIOR_VERBOSE_PRINT(DEBUG_LIFT_LOAD_TEST_BEHAVIOR, "BehaviorLiftLoadTest.SetState",
                          "set state to '%s'", GetDebugStateName().c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* BehaviorLiftLoadTest::StateToString(const State m)
{
  switch(m) {
    case State::Init:
      return "Init";
    case State::WaitForPutdown:
      return "WaitForPutdown";
    case State::TestComplete:
      return "TestComplete";
    default: return nullptr;
  }
  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::UpdateStateName()
{
  std::string name = StateToString(_dVars.currentState);
  
  name += std::to_string(_dVars.numLiftRaises);
  
  if( IsControlDelegated() ) {
    name += '*';
  }
  else {
    name += ' ';
  }
  
  SetDebugStateName(name);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotOffTreadsStateChanged:
    {
      if(event.GetData().Get_RobotOffTreadsStateChanged().treadsState != OffTreadsState::OnTreads){
        SetCurrState(State::WaitForPutdown);
      }
      break;
    }
      
    default:
      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.HandleWhileRunning.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::AlwaysHandleInScope(const GameToEngineEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::SetLiftLoadTestAsActivatable:
    {
      _dVars.canRun = true;
      break;
    }
    default:
    {
      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.AlwaysHandle.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::HandleWhileActivated(const RobotToEngineEvent& event)
{
  switch(event.GetData().GetTag()) {
    case RobotInterface::RobotToEngineTag::liftLoad:
    {
      const RobotInterface::LiftLoad& payload = event.GetData().Get_liftLoad();
      
      _dVars.loadStatusReceived = true;
      _dVars.hasLoad = payload.hasLoad;
      if (_dVars.hasLoad) {
        ++_dVars.numHadLoad;
      }
      ++_dVars.numLiftRaises;
      PRINT_CH_DEBUG("Behaviors", "BehaviorLiftLoadTest.HandleLiftLoad.HasLoad", "%d / %d", _dVars.numHadLoad, _dVars.numLiftRaises);
      break;
    }
    default:
    {
      PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.HandleWhileRunning.InvalidRobotToEngineTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::Write(const std::string& s)
{
  if(_iConfig.logger != nullptr){
    _iConfig.logger->Write(s + "\n");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLiftLoadTest::PrintStats()
{
  u32 loadPercent = (100*_dVars.numHadLoad) / _dVars.numLiftRaises;
  
  PRINT_CH_DEBUG("Behaviors", "BehaviorLiftLoadTest.PrintStats.LoadRate", "%d", loadPercent);
  
  std::stringstream ss;
  ss << "*****************\n";
  ss << "Load Detected:    " << _dVars.numHadLoad << "\n";
  ss << "No load detected: " << _dVars.numLiftRaises - _dVars.numHadLoad << "\n";
  ss << "LoadRate:         " << loadPercent << "%\n";
  Write(ss.str());
  Write("=====End LiftLoadTest=====");
}

} // namespace Vector
} // namespace Anki
