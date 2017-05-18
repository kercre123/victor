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


#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviors/behaviorLiftLoadTest.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"
#include "util/console/consoleInterface.h"
#include <ctime>

#define DEBUG_LIFT_LOAD_TEST_BEHAVIOR 1

#define END_TEST_IN_HANDLER(RESULT, NAME) EndAttempt(robot, RESULT, NAME); return;

namespace{
  // This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
  // All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
} while(0) \

static const char* kBehaviorTestName = "LiftLoadTest";
}


namespace Anki {
  namespace Cozmo {
    
    CONSOLE_VAR(u32, kNumLiftRaises,              "LiftLoadTest", 50);
    
    // !!! These should match the values in PickAndPlaceController for accurate testing
    static const f32 DEFAULT_LIFT_SPEED_RAD_PER_SEC = 1.5;
    static const f32 DEFAULT_LIFT_ACCEL_RAD_PER_SEC2 = 10;

    
    BehaviorLiftLoadTest::BehaviorLiftLoadTest(Robot& robot, const Json::Value& config)
    : IBehavior(robot, config)
    , _logger(nullptr, robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, "liftLoadTest"))
    {
      
      SubscribeToTags({
        EngineToGameTag::RobotOffTreadsStateChanged
      });
      
      SubscribeToTags({
        GameToEngineTag::SetLiftLoadTestAsRunnable
      });
      
      SubscribeToTags(robot.GetID(), {
        RobotInterface::RobotToEngineTag::liftLoad
      });
    }
    
    bool BehaviorLiftLoadTest::IsRunnableInternal(const BehaviorPreReqNone& preReqData ) const
    {
      return _canRun && (_currentState == State::Init || _currentState == State::TestComplete);
    }
    
    Result BehaviorLiftLoadTest::InitInternal(Robot& robot)
    {
      robot.GetBehaviorManager().DisableReactionsWithLock(
                                     kBehaviorTestName,
                                     ReactionTriggerHelpers::kAffectAllArray);
      

      _abortTest = false;
      _currentState = State::Init;
      _numLiftRaises = 0;
      _numHadLoad = 0;

      robot.GetActionList().Cancel();
      
     
      Write("=====Start LiftLoadTest=====");
      const RobotInterface::FWVersionInfo& fw = robot.GetRobotToEngineImplMessaging().GetFWVersionInfo();
      std::stringstream ss;
      ss << "RobotSN: " << robot.GetHeadSerialNumber() << "\n";
      ss << "Battery(V): " << robot.GetBatteryVoltage() << "\n\n";
      ss << "Firmware Version\n";
      ss << "Body Version: " << std::hex << fw.bodyVersion << "\n";
      ss << "RTIP Version: " << std::hex << fw.rtipVersion << "\n";
      ss << "Wifi Version: " << std::hex << fw.wifiVersion << "\n";
      ss << "ToEngineCLADHash: ";
      for(auto iter = fw.toEngineCLADHash.begin(); iter != fw.toEngineCLADHash.end(); ++iter)
      {
        ss << std::hex << (int)(*iter) << " ";
      }
      ss << "\nToRobotCLADHash: ";
      for(auto iter = fw.toRobotCLADHash.begin(); iter != fw.toRobotCLADHash.end(); ++iter)
      {
        ss << std::hex << (int)(*iter) << " ";
      }
      ss << "\n";
      Write(ss.str());
      
      
      return RESULT_OK;
    }
    
    IBehavior::Status BehaviorLiftLoadTest::UpdateInternal(Robot& robot)
    {
      if(_numLiftRaises == kNumLiftRaises || _abortTest)
      {
        if (_numLiftRaises == kNumLiftRaises) {
          PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.UpdateInternal.TestComplete", "%d", _numLiftRaises);
          Write("\nTest Completed Successfully");
        } else {
          PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.UpdateInternal.TestAborted", "%d", _numLiftRaises);
          Write("\nTest Aborted");
        }
        
        PrintStats();
        _abortTest = false;
        _numLiftRaises = 0;
        _canRun = false;
        
        SetCurrState(State::TestComplete);
        return Status::Complete;
      }
      
      if(IsActing())
      {
        return Status::Running;
      }
      
      switch(_currentState)
      {
        case State::Init:
        {
          auto lowerLiftAction = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
          lowerLiftAction->SetMaxLiftSpeed(DEFAULT_LIFT_SPEED_RAD_PER_SEC);
          lowerLiftAction->SetLiftAccel(DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
          
          auto raiseLiftAction = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_CARRY);
          raiseLiftAction->SetMaxLiftSpeed(DEFAULT_LIFT_SPEED_RAD_PER_SEC);
          raiseLiftAction->SetLiftAccel(DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
          
          CompoundActionSequential* compoundAction = new CompoundActionSequential(robot, { lowerLiftAction, raiseLiftAction });
          
          StartActing(compoundAction,
                      [this, &robot](ActionResult result){
                        if (ActionResult::SUCCESS == result) {
                          // Tell robot to check for load and wait for response
                          robot.SendRobotMessage<RobotInterface::CheckLiftLoad>();
                          
                          PRINT_CH_INFO("Behaviors", "BehaviorLiftLoadTest.WaitingForLiftLoadMsg", "");
                          _loadStatusReceived = false;
                          auto waitForLiftLoadMsgLambda = [this](Robot& robot) {
                            return _loadStatusReceived;
                          };
                          auto waitAction = new WaitForLambdaAction(robot, waitForLiftLoadMsgLambda);
                          StartActing(waitAction);
                          
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
          if(robot.GetOffTreadsState() == OffTreadsState::OnTreads){
            _abortTest = true;
          }
          break;
        }
        case State::TestComplete:
        {
          return Status::Complete;
        }
        default:
        {
          PRINT_NAMED_ERROR("BehaviorLiftLoadTest.Update.UnknownState", "Reached unknown state %d", (u32)_currentState);
          return Status::Failure;
        }
      }
      return Status::Running;
    }
    
    void BehaviorLiftLoadTest::StopInternal(Robot& robot)
    {
      robot.GetBehaviorManager().RemoveDisableReactionsLock(kBehaviorTestName);
    }
    
    void BehaviorLiftLoadTest::SetCurrState(State s)
    {
      _currentState = s;
      
      UpdateStateName();
      
      BEHAVIOR_VERBOSE_PRINT(DEBUG_LIFT_LOAD_TEST_BEHAVIOR, "BehaviorLiftLoadTest.SetState",
                             "set state to '%s'", GetDebugStateName().c_str());
    }

    
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
    
    void BehaviorLiftLoadTest::UpdateStateName()
    {
      std::string name = StateToString(_currentState);
      
      name += std::to_string(_numLiftRaises);
      
      if( IsActing() ) {
        name += '*';
      }
      else {
        name += ' ';
      }
      
      SetDebugStateName(name);
    }
    
    void BehaviorLiftLoadTest::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
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
          PRINT_NAMED_INFO("BehaviorLiftLoadTest.HandleWhileRunning.InvalidTag",
                           "Received unexpected event with tag %hhu.", event.GetData().GetTag());
          break;
      }
    }
    
    void BehaviorLiftLoadTest::AlwaysHandle(const GameToEngineEvent& event, const Robot& robot)
    {
      switch(event.GetData().GetTag())
      {
        case GameToEngineTag::SetLiftLoadTestAsRunnable:
        {
          _canRun = true;
          break;
        }
        default:
        {
          PRINT_NAMED_INFO("BehaviorLiftLoadTest.AlwaysHandle.InvalidTag",
                           "Received unexpected event with tag %hhu.", event.GetData().GetTag());
          break;
        }
      }
    }
    
    void BehaviorLiftLoadTest::HandleWhileRunning(const RobotToEngineEvent& event, const Robot& robot)
    {
      switch(event.GetData().GetTag()) {
        case RobotInterface::RobotToEngineTag::liftLoad:
        {
          const RobotInterface::LiftLoad& payload = event.GetData().Get_liftLoad();
          
          _loadStatusReceived = true;
          _hasLoad = payload.hasLoad;
          if (_hasLoad) {
            ++_numHadLoad;
          }
          ++_numLiftRaises;
          PRINT_NAMED_DEBUG("BehaviorLiftLoadTest.HandleLiftLoad.HasLoad", "%d / %d", _numHadLoad, _numLiftRaises);
          break;
        }
        default:
        {
          PRINT_NAMED_INFO("BehaviorLiftLoadTest.HandleWhileRunning.InvalidRobotToEngineTag",
                           "Received unexpected event with tag %hhu.", event.GetData().GetTag());
          break;
        }
      }
    }
    
    void BehaviorLiftLoadTest::Write(const std::string& s)
    {
      _logger.Write(s + "\n");
    }
    
    void BehaviorLiftLoadTest::PrintStats()
    {
      u32 loadPercent = (100*_numHadLoad) / _numLiftRaises;
      
      PRINT_NAMED_DEBUG("BehaviorLiftLoadTest.PrintStats.LoadRate", "%d", loadPercent);
      
      std::stringstream ss;
      ss << "*****************\n";
      ss << "Load Detected:    " << _numHadLoad << "\n";
      ss << "No load detected: " << _numLiftRaises - _numHadLoad << "\n";
      ss << "LoadRate:         " << loadPercent << "%\n";
      Write(ss.str());
      Write("=====End LiftLoadTest=====");
    }
  }
}
