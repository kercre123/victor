/**
 * File: behaviorDockingTestSimple.cpp
 *
 * Author: Al Chaussee
 * Date:   05/09/2016
 *
 * Description: Simple docking test behavior
 *              Saves attempt information to log webotsCtrlGameEngine/temp
 *              Saves images with timestamps in webotsCtrlViz/images
 *
 *              Init conditions:
 *                - Cozmo is about 154mm away from center of a cube facing it
 *
 *              Behavior:
 *                - Pick up cube
 *                - Place cube
 *                - Drive back to start pose with random offsets
 *                - Repeat until consecutively fails kMaxConsecFails times or does kMaxNumAttempts attempts
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviors/behaviorDockingTestSimple.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "clad/types/dockingSignals.h"
#include "util/console/consoleInterface.h"
#include <ctime>

#define DEBUG_DOCKING_TEST_BEHAVIOR 1

#define END_TEST_IN_HANDLER(RESULT, NAME) EndAttempt(robot, RESULT, NAME); return;

namespace Anki {
  namespace Cozmo {
  
    CONSOLE_VAR(u32, kMaxNumAttempts,              "DockingTest", 30);
    CONSOLE_VAR(u32, kMaxConsecFails,              "DockingTest", 3);
    CONSOLE_VAR(u32, kTestDockingMethod,           "DockingTest", (u8)DockingMethod::HYBRID_DOCKING);
    CONSOLE_VAR(f32, kMaxXAwayFromPreDock_mm,      "DockingTest", 30);
    CONSOLE_VAR(f32, kMaxYAwayFromPreDock_mm,      "DockingTest", 50);
    CONSOLE_VAR(f32, kMaxAngleAwayFromPreDock_deg, "DockingTest", 10);
    CONSOLE_VAR(bool, kDriveToAndPickupBlock,      "DockingTest", true);
    CONSOLE_VAR(bool, kRollInsteadOfPickup,        "DockingTest", false);
    CONSOLE_VAR(bool, kDoDeepRoll,                 "DockingTest", false);
    
    static const size_t NUM_LIGHTS = (size_t)LEDId::NUM_BACKPACK_LEDS;
    static const std::array<u32,NUM_LIGHTS> pass_onColor{{NamedColors::BLACK,NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> pass_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> pass_onPeriod_ms{{1000,1000,1000,1000,1000}};
    static const std::array<u32,NUM_LIGHTS> pass_offPeriod_ms{{100,100,100,100,100}};
    static const std::array<u32,NUM_LIGHTS> pass_transitionOnPeriod_ms{{450,450,450,450,450}};
    static const std::array<u32,NUM_LIGHTS> pass_transitionOffPeriod_ms{{450,450,450,450,450}};
    
    static const std::array<u32,NUM_LIGHTS> fail_onColor{{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> fail_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> fail_onPeriod_ms{{500,500,500,500,500}};
    static const std::array<u32,NUM_LIGHTS> fail_offPeriod_ms{{500,500,500,500,500}};
    static const std::array<u32,NUM_LIGHTS> fail_transitionOnPeriod_ms{};
    static const std::array<u32,NUM_LIGHTS> fail_transitionOffPeriod_ms{};
    
    BehaviorDockingTestSimple::BehaviorDockingTestSimple(Robot& robot, const Json::Value& config)
    : IBehavior(robot, config)
    , _initialVisionMarker(Vision::MARKER_UNKNOWN)
    , _markerBeingSeen(Vision::MARKER_UNKNOWN)
    , _cubePlacementPose(Radians(DEG_TO_RAD(0)), Z_AXIS_3D(), {176, 0, 22}, &robot.GetPose().FindOrigin())
    , _logger(robot.GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "dockingTest"))
    {
      SetDefaultName("DockingTestSimple");
      
      _motionProfile = PathMotionProfile();
      _motionProfile.isCustom = true;
      
      SubscribeToTags({{
        EngineToGameTag::RobotCompletedAction,
        EngineToGameTag::RobotObservedObject,
        EngineToGameTag::RobotDeletedObject,
        EngineToGameTag::ObjectMovedWrapper,
        EngineToGameTag::RobotStopped,
        EngineToGameTag::RobotPutDown
      }});
      
      if(nullptr != robot.GetContext()->GetRobotManager() &&
         robot.GetContext()->GetRobotManager()->GetMsgHandler() != nullptr)
      {
        _signalHandle = (robot.GetContext()->GetRobotManager()->GetMsgHandler()->Subscribe(robot.GetID(),
            RobotInterface::RobotToEngineTag::dockingStatus,
            std::bind(&BehaviorDockingTestSimple::HandleDockingStatus, this, std::placeholders::_1)));
      }
    }
    
    bool BehaviorDockingTestSimple::IsRunnableInternal(const Robot& robot) const
    {
      return _currentState == State::Init;
    }
    
    Result BehaviorDockingTestSimple::InitInternal(Robot& robot)
    {
      robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::EnableReactionaryBehaviors>(false);
    
      _currentState = State::Init;
      _numFails = 0;
      _numDockingRetries = 0;
      _numAttempts = 0;
      _numConsecFails = 0;
      _didHM = false;
      _failedCurrentAttempt = false;
    
      _actionCallbackMap.clear();
      robot.GetActionList().Cancel();
      
      time_t t = time(0);
      struct tm* now = localtime(&t);
      
      Write("=====Start DockingTestSimple=====");
      const RobotInterface::FWVersionInfo& fw = robot.GetFWVersionInfo();
      std::stringstream ss;
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
      
      ss.clear();
      ss << "@" << kMaxXAwayFromPreDock_mm << " " << kMaxYAwayFromPreDock_mm << " " << kMaxAngleAwayFromPreDock_deg;
      ss << "\n";
      Write(ss.str());
      
      if(robot.GetContext()->GetVizManager() != nullptr)
      {
        std::stringstream ss;
        ss << "images_" << now->tm_mon+1 << "-" << now->tm_mday << "-" << now->tm_hour << "." << now->tm_min;
        _imageFolder = ss.str();
        robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Stream, ss.str());
      }
      
      return RESULT_OK;
    }
    
    IBehavior::Status BehaviorDockingTestSimple::UpdateInternal(Robot& robot)
    {
      if(_numAttempts == kMaxNumAttempts && _currentState != State::ManualReset)
      {
        Write("\nTest Completed Successfully");
        
        PrintStats();
        
        if(robot.GetContext()->GetVizManager() != nullptr)
        {
          robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Stream);
        }
        
        _yellForCompletion = true;
        SetCurrStateAndFlashLights(State::ManualReset, robot);
      }
    
      if(IsActing())
      {
        return Status::Running;
      }
      
      switch(_currentState)
      {
        case State::Init:
        {
          // Cancel all actions
          for (const auto& tag : _actionCallbackMap) {
            robot.GetActionList().Cancel(tag.first);
          }
          _actionCallbackMap.clear();
          
          if(robot.GetContext()->GetVizManager() != nullptr)
          {
            robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Stream, _imageFolder);
          }
        
          // Turn off backpack lights in case we needed to be manually reset
          robot.SetBackpackLights(fail_offColor, fail_offColor,
                                  fail_onPeriod_ms, fail_offPeriod_ms,
                                  fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
          
          _blockObjectIDPickup.UnSet();
          
          _cubePlacementPose = Pose3d(Radians(DEG_TO_RAD(0)), Z_AXIS_3D(), {176, 0, 22}, &robot.GetPose().FindOrigin());
          
          CompoundActionSequential* action = new CompoundActionSequential(robot, {new MoveHeadToAngleAction(robot, 0, DEG_TO_RAD(1), 0), new WaitAction(robot, 2)});
          StartActing(robot, action,
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        if (result != ActionResult::SUCCESS) {
                          EndAttempt(robot, result, "MoveHead", true);
                          SetCurrState(State::Init);
                          return false;
                        }
                        SetCurrState(State::Inactive);
                        return true;
                      });
          break;
        }
        case State::Inactive:
        {
          _numAttempts++;
          _didHM = false;
          _numDockingRetries = 0;
          _failedCurrentAttempt = false;
          _attemptStartTime = robot.GetRobotState().lastImageTimeStamp;
          
          if(kRollInsteadOfPickup)
          {
            SetCurrState(State::Roll);
          }
          else
          {
            SetCurrState(State::PickupLow);
          }
          _numSawObject = 0;
          break;
        }
        case State::Roll:
        {
          if(_blockObjectIDPickup.IsSet())
          {
            _initialCubePose = robot.GetBlockWorld().GetObjectByID(_blockObjectIDPickup)->GetPose();
            _initialRobotPose = robot.GetPose();
            
            ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_blockObjectIDPickup);
            Block* block = dynamic_cast<Block*>(object);
            Pose3d junk;
            _initialVisionMarker = const_cast<Vision::KnownMarker&>(block->GetTopMarker(junk));
            
           
            DriveToObjectAction* driveAction = new DriveToObjectAction(robot, _blockObjectIDPickup, PreActionPose::ROLLING);
            driveAction->SetMotionProfile(_motionProfile);
            StartActing(robot, driveAction,
                        [this, &robot](const ActionResult& result, const ActionCompletedUnion& completionUnion){
                          if(result == ActionResult::SUCCESS)
                          {
                            _initialRobotPose = robot.GetPose();
                            
                            RollObjectAction* action = new RollObjectAction(robot, _blockObjectIDPickup);
                            action->SetDockingMethod((DockingMethod)kTestDockingMethod);
                            action->EnableDeepRoll(kDoDeepRoll);
                            
                            StartActing(robot, action,
                                        [this, &robot](const ActionResult& result, const ActionCompletedUnion& completedUnion){
                                          if (result != ActionResult::SUCCESS) {
                                            if(_numSawObject < 5)
                                            {
                                              EndAttempt(robot, result, "RollNotSeeingObject", true);
                                            }
                                            else
                                            {
                                              EndAttempt(robot, result, "Roll", true);
                                            }
                                            return false;
                                          }
                                          SetCurrState(State::Reset);
                                          return true;
                                        });
                            return true;
                          }
                          else
                          {
                            EndAttempt(robot, result, "DriveToObject", true);
                            return false;
                          }
                          return false;
                        });
           
          }
          else
          {
            Write("\nPickupBlockID never set");
            _yellForHelp = true;
            SetCurrStateAndFlashLights(State::ManualReset, robot);
          }
          break;
        }
        case State::PickupLow:
        {
          if(_blockObjectIDPickup.IsSet())
          {
            _initialCubePose = robot.GetBlockWorld().GetObjectByID(_blockObjectIDPickup)->GetPose();
            _initialRobotPose = robot.GetPose();
          
            if(kDriveToAndPickupBlock)
            {
              DriveToObjectAction* driveAction = new DriveToObjectAction(robot, _blockObjectIDPickup, PreActionPose::DOCKING);
              driveAction->SetMotionProfile(_motionProfile);
              StartActing(robot, driveAction,
                          [this, &robot](const ActionResult& result, const ActionCompletedUnion& completionUnion){
                              if(result == ActionResult::SUCCESS)
                              {
                                _initialRobotPose = robot.GetPose();
                                PickupObjectAction* action = new PickupObjectAction(robot, _blockObjectIDPickup);
                                action->SetDockingMethod((DockingMethod)kTestDockingMethod);
                                StartActing(robot, action,
                                            [this, &robot](const ActionResult& result, const ActionCompletedUnion& completedUnion){
                                              if (result != ActionResult::SUCCESS) {
                                                if(_numSawObject < 5)
                                                {
                                                  EndAttempt(robot, result, "PickupNotSeeingObject", true);
                                                }
                                                else
                                                {
                                                  EndAttempt(robot, result, "Pickup", true);
                                                }
                                                return false;
                                              }
                                              SetCurrState(State::PlaceLow);
                                              return true;
                                            });
                                return true;
                              }
                              else
                              {
                                EndAttempt(robot, result, "DriveToObject", true);
                                return false;
                              }
                            return false;
                          });
            }
            else
            {
              PickupObjectAction* action = new PickupObjectAction(robot, _blockObjectIDPickup);
              action->SetDockingMethod((DockingMethod)kTestDockingMethod);
              action->SetDoNearPredockPoseCheck(false);
              StartActing(robot, action,
                          [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                            if (result != ActionResult::SUCCESS) {
                              if(_numSawObject < 5)
                              {
                                EndAttempt(robot, result, "PickupNotSeeingObject", true);
                              }
                              else
                              {
                                EndAttempt(robot, result, "Pickup", true);
                              }
                              return false;
                            }
                            SetCurrState(State::PlaceLow);
                            return true;
                          });
            }
          }
          else
          {
            Write("\nPickupBlockID never set");
            _yellForHelp = true;
            SetCurrStateAndFlashLights(State::ManualReset, robot);
          }
          break;
        }
        case State::PlaceLow:
        {
          PlaceObjectOnGroundAtPoseAction* action = new PlaceObjectOnGroundAtPoseAction(robot, _cubePlacementPose);
          action->SetMotionProfile(_motionProfile);
          StartActing(robot, action,
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        if (result != ActionResult::SUCCESS) {
                          EndAttempt(robot, result, "PlaceOnGround", true);
                          return false;
                        }
                        SetCurrState(State::Reset);
                        return true;
                      });
          break;
        }
        case State::Reset:
        {
          f32 randX = GetRNG().RandDblInRange(-kMaxXAwayFromPreDock_mm, kMaxXAwayFromPreDock_mm);
          f32 randY = GetRNG().RandDblInRange(-kMaxYAwayFromPreDock_mm, kMaxYAwayFromPreDock_mm);
          f32 randA = GetRNG().RandDblInRange(-DEG_TO_RAD(kMaxAngleAwayFromPreDock_deg),
                                         DEG_TO_RAD(kMaxAngleAwayFromPreDock_deg));
          
          // Get the preActionPose relating to the marker we
          ActionableObject* aObject = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_blockObjectIDPickup));
          
          if(aObject == nullptr)
          {
            PRINT_NAMED_INFO("BehaviorDockingTest.Reset", "ActionableObject is null");
            return Status::Failure;
          }
          
          std::vector<PreActionPose> preActionPoses;
          std::vector<std::pair<Quad2f, ObjectID> > obstacles;
          robot.GetBlockWorld().GetObstacles(obstacles);
          aObject->GetCurrentPreActionPoses(preActionPoses,
                                            {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                            {_initialVisionMarker.GetCode()},
                                            obstacles,
                                            nullptr);
          
          if(preActionPoses.size() != 1)
          {
            PRINT_NAMED_INFO("BehaviorDockingTest.Reset", "Found %i preActionPoses for marker %s",
                             (int)preActionPoses.size(),
                             _initialVisionMarker.GetCodeName());
            
            // If we are rolling and we can't find any preAction poses it means the block was not rolled
            // so get preActionPoses for the marker we are currently seeing
            if(kRollInsteadOfPickup)
            {
              aObject->GetCurrentPreActionPoses(preActionPoses,
                                                {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                                {_markerBeingSeen.GetCode()},
                                                obstacles,
                                                nullptr);
              
              if(preActionPoses.size() != 1)
              {
                PRINT_NAMED_INFO("BehaviorDockingTest.Reset", "Found %i preActionPoses for marker %s",
                                 (int)preActionPoses.size(),
                                 _markerBeingSeen.GetCodeName());
                return Status::Failure;
              }
            }
            else
            {
              return Status::Failure;
            }
          }
          
          Pose3d preActionPose = preActionPoses.front().GetPose();
          f32 x = preActionPose.GetTranslation().x();
          f32 y = preActionPose.GetTranslation().y();
          f32 angle = preActionPose.GetRotation().GetAngleAroundZaxis().ToFloat();
          
          // If we are reseting due to an action failing then we shouldn't go to a random offset
          // and we should not count this reset pickup as an attempt
          if(_endingFromFailedAction)
          {
            randX = 0;
            randY = 0;
            randA = 0;
            _endingFromFailedAction = false;
            _numAttempts--;
            _numExtraAttemptsDueToFailure++;
          }
          
          Pose3d p(Radians(angle + randA), Z_AXIS_3D(), {x + randX, y + randY, 0}, &robot.GetPose().FindOrigin());
          
          DriveToPoseAction* action = new DriveToPoseAction(robot, p);
          
          action->SetMotionProfile(_motionProfile);
          StartActing(robot, action,
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        if (result != ActionResult::SUCCESS) {
                          EndAttempt(robot, result, "DriveToPose", true);
                          return false;
                        }
                        SetCurrState(State::Inactive);
                        if(!_failedCurrentAttempt)
                        {
                          RecordAttempt(result, "");
                          _numConsecFails = 0;
                        }
                        return true;
                      });
          break;
        }
        case State::ManualReset:
        {
          _reset = true;
          
          if(_yellForCompletion)
          {
            _yellForCompletion = false;
            
            if(robot.GetContext()->GetVizManager() != nullptr)
            {
              robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Off);
            }
            
            IActionRunner* action = new CompoundActionSequential(robot, {new SayTextAction(robot, "Test Complete", SayTextStyle::Normal, true), new WaitAction(robot, 3)});
            StartActing(robot, action,
                        [this](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                          if(result == ActionResult::SUCCESS)
                          {
                            _yellForCompletion = true;
                          }
                          return true;
                        });
          }
          else if(_yellForHelp)
          {
            _yellForHelp = false;
            
            if(robot.GetContext()->GetVizManager() != nullptr)
            {
              robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Off);
            }
            
            IActionRunner* action = new CompoundActionSequential(robot, {new SayTextAction(robot, "Help", SayTextStyle::Normal, true), new WaitAction(robot, 3)});
            StartActing(robot, action,
                        [this](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                          if(result == ActionResult::SUCCESS)
                          {
                            _yellForHelp = true;
                          }
                          return true;
                        });
          }
          
          break;
        }
        default:
        {
          PRINT_NAMED_ERROR("BehaviorDockingTest.Update.UnknownState", "Reached unknown state %d", (u32)_currentState);
          return Status::Failure;
        }
      }
      return Status::Running;
    }
    
    void BehaviorDockingTestSimple::StopInternal(Robot& robot)
    {
      robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::EnableReactionaryBehaviors>(true);
    
      // Cancel all actions
      for (const auto& tag : _actionCallbackMap) {
        robot.GetActionList().Cancel(tag.first);
      }
      _actionCallbackMap.clear();
      
      PrintStats();
      
      if(robot.GetContext()->GetVizManager() != nullptr)
      {
        robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Off);
      }
    }
    
    void BehaviorDockingTestSimple::SetCurrState(State s)
    {
      _currentState = s;
      
      UpdateStateName();
      
      BEHAVIOR_VERBOSE_PRINT(DEBUG_DOCKING_TEST_BEHAVIOR, "BehaviorDockingTest.SetState",
                             "set state to '%s'", GetStateName().c_str());
    }
    
    void BehaviorDockingTestSimple::SetCurrStateAndFlashLights(State s, Robot& robot)
    {
      if(_yellForHelp)
      {
        robot.SetBackpackLights(fail_onColor, fail_offColor,
                                fail_onPeriod_ms, fail_offPeriod_ms,
                                fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
      }
      else if(_yellForCompletion)
      {
        robot.SetBackpackLights(pass_onColor, pass_offColor,
                                pass_onPeriod_ms, pass_offPeriod_ms,
                                pass_transitionOnPeriod_ms, pass_transitionOffPeriod_ms);
      }
      SetCurrState(s);
    }
    
    const char* BehaviorDockingTestSimple::StateToString(const State m)
    {
      switch(m) {
        case State::Init:
          return "Init";
        case State::Inactive:
          return "Inactive";
        case State::Roll:
          return "Roll";
        case State::PickupLow:
          return "PickupLow";
        case State::PlaceLow:
          return "PlaceLow";
        case State::Reset:
          return "Reset";
        case State::ManualReset:
          return "ManualReset";

        default: return nullptr;
      }
      return nullptr;
    }
    
    void BehaviorDockingTestSimple::UpdateStateName()
    {
      std::string name = StateToString(_currentState);
      
      name += std::to_string(_numAttempts+_numExtraAttemptsDueToFailure);
      
      if(_currentState == State::Reset && _endingFromFailedAction)
      {
        name += "FromFailure";
      }
      
      if( IsActing() ) {
        name += '*';
      }
      else {
        name += ' ';
      }
      
      SetStateName(name);
    }
    
    void BehaviorDockingTestSimple::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
    {
      switch(event.GetData().GetTag())
      {
        case EngineToGameTag::RobotCompletedAction:
          HandleActionCompleted(robot, event.GetData().Get_RobotCompletedAction());
          break;
          
        case EngineToGameTag::RobotObservedObject:
          HandleObservedObject(robot, event.GetData().Get_RobotObservedObject());
          break;
          
        case EngineToGameTag::RobotStopped:
          HandleRobotStopped(robot, event.GetData().Get_RobotStopped());
          break;
          
        case EngineToGameTag::RobotPutDown:
        {
          _currentState = State::Init;
          
          _numDockingRetries = 0;
          _numConsecFails = 0;
          _didHM = false;
          _failedCurrentAttempt = false;
          
          
          if(_reset)
          {
            Write("\n\n------Test Manually Reset------\n");
          }
          else
          {
            _numAttempts = 0;
            _numFails = 0;
          }
          _yellForCompletion = false;
          _yellForHelp = false;
          _reset = false;
          break;
        }
          
        default:
          PRINT_NAMED_INFO("BehaviorDockingTest.HandleWhileRunning.InvalidTag",
                            "Received unexpected event with tag %hhu.", event.GetData().GetTag());
          break;
      }
    }
    
    void BehaviorDockingTestSimple::HandleActionCompleted(Robot& robot,
                                                      const ExternalInterface::RobotCompletedAction &msg)
    {
      if(!IsRunning()) {
        // Ignore action completion messages while not running
        return;
      }
      
      auto callback = _actionCallbackMap.find(msg.idTag);
      
      if(callback == _actionCallbackMap.end())
      {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_DOCKING_TEST_BEHAVIOR, "BehaviorDockingTest.HandleActionCompleted.OtherAction",
                               "finished action id=%d type=%s but don't care",
                               msg.idTag, EnumToString(msg.actionType));
      }
      else
      {
        if(callback->second)
        {
          callback->second(msg.result, msg.completionInfo);
        }
        _actionCallbackMap.erase(callback);
      }
      
    } // HandleActionCompleted()
    
    void BehaviorDockingTestSimple::EndAttempt(Robot& robot, ActionResult result, std::string name, bool endingFromFailedAction)
    {
      _endingFromFailedAction = endingFromFailedAction;
      RecordAttempt(result, name);
      SetCurrState(State::Reset);
      if(result != ActionResult::SUCCESS)
      {
        if(!_failedCurrentAttempt)
        {
          _numFails++;
        }
        _failedCurrentAttempt = true;
        if(++_numConsecFails >= kMaxConsecFails)
        {
          Write("\nReached max number of consecutive failed attempts");
          _yellForHelp = true;
          SetCurrStateAndFlashLights(State::ManualReset, robot);
        }
      }
      else
      {
        _numConsecFails = 0;
      }
    }
    
    void BehaviorDockingTestSimple::HandleObservedObject(Robot& robot,
                                                     const ExternalInterface::RobotObservedObject &msg)
    {
      ObjectID objectID = msg.objectID;
      const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);
      
      if(oObject->GetFamily() == ObjectFamily::LightCube)
      {
        if(_blockObjectIDPickup.IsSet() && _blockObjectIDPickup == objectID)
        {
          _numSawObject++;
          
          std::vector<const Vision::KnownMarker*> observedMarkers;
          oObject->GetObservedMarkers(observedMarkers);
          if(observedMarkers.size() == 1)
          {
            _markerBeingSeen = observedMarkers.front()->GetCode();
          }
          else
          {
            PRINT_NAMED_INFO("BehaviorDockingTest.HandleObservedObject", "Saw more than one marker");
            _yellForHelp = true;
            END_TEST_IN_HANDLER(ActionResult::FAILURE_ABORT, "SawMoreThanOneMarkerOnInit");
          }
          return;
        }
        else if(!_blockObjectIDPickup.IsSet())
        {
          _blockObjectIDPickup = objectID;
          std::vector<const Vision::KnownMarker*> observedMarkers;
          oObject->GetObservedMarkers(observedMarkers);
          if(observedMarkers.size() == 1)
          {
            _initialVisionMarker = observedMarkers.front()->GetCode();
          }
          else
          {
            PRINT_NAMED_INFO("BehaviorDockingTest.HandleObservedObject", "Saw more than one marker");
            _yellForHelp = true;
            END_TEST_IN_HANDLER(ActionResult::FAILURE_ABORT, "SawMoreThanOneMarkerOnInit");
          }
          return;
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorDockingTest.HandleObservedObject.UnexpectedBlock", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          _yellForHelp = true;
          END_TEST_IN_HANDLER(ActionResult::FAILURE_ABORT, "UnexpectedBlock");
        }
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorDockingTest.HandleObservedObject.UnexpectedObservedObject", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
        _yellForHelp = true;
        END_TEST_IN_HANDLER(ActionResult::FAILURE_ABORT, "UnexpectedObject");
      }
    }
    
    void BehaviorDockingTestSimple::HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg)
    {
      EndAttempt(robot, ActionResult::FAILURE_ABORT, "RobotStopped");
      _yellForHelp = true;
      SetCurrStateAndFlashLights(State::ManualReset, robot);
    }
    
    void BehaviorDockingTestSimple::HandleDockingStatus(const AnkiEvent<RobotInterface::RobotToEngine>& message)
    {
      const DockingStatus& payload = message.GetData().Get_dockingStatus();
      switch(payload.status)
      {
        case(Anki::Cozmo::Status::STATUS_BACKING_UP):
        {
          _numDockingRetries++;
          break;
        }
        case(Anki::Cozmo::Status::STATUS_DOING_HANNS_MANEUVER):
        {
          _didHM = true;
          break;
        }
      }
    }
    
    void BehaviorDockingTestSimple::StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback)
    {
      assert(_actionCallbackMap.count(action->GetTag()) == 0);
      
      if (robot.GetActionList().QueueActionNow(action) == RESULT_OK) {
        _actionCallbackMap[action->GetTag()] = callback;
      } else {
        PRINT_NAMED_WARNING("BehaviorDockingTest.StartActing.QueueActionFailed", "Action type %s", EnumToString(action->GetType()));
        EndAttempt(robot, ActionResult::FAILURE_ABORT, "QueueActionFailed");
      }
    }
    
    void BehaviorDockingTestSimple::RecordAttempt(ActionResult result, std::string name)
    {
      std::stringstream ss;
      ss << "Attempt: " << _numAttempts+_numExtraAttemptsDueToFailure << " TimeStarted: " << _attemptStartTime << " Result: " << EnumToString(result);
      ss << " " << name << " DockingMethod: " << (int)kTestDockingMethod;
      ss << " HM: " << _didHM << " NumDockingRetries: " << _numDockingRetries << "\n\t\t";
      
      char buf[300];
      sprintf(buf, "Translation=(%f, %f %f), RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg)",
              _initialRobotPose.GetTranslation().x(),
              _initialRobotPose.GetTranslation().y(),
              _initialRobotPose.GetTranslation().z(),
              _initialRobotPose.GetRotationAxis().x(),
              _initialRobotPose.GetRotationAxis().y(),
              _initialRobotPose.GetRotationAxis().z(),
              _initialRobotPose.GetRotationAngle().ToFloat(),
              _initialRobotPose.GetRotationAngle().getDegrees());
      
      ss << "InitialRobotPose " << buf << "\n\t\t";
      
      sprintf(buf, "Translation=(%f, %f %f), RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg)",
              _initialCubePose.GetTranslation().x(),
              _initialCubePose.GetTranslation().y(),
              _initialCubePose.GetTranslation().z(),
              _initialCubePose.GetRotationAxis().x(),
              _initialCubePose.GetRotationAxis().y(),
              _initialCubePose.GetRotationAxis().z(),
              _initialCubePose.GetRotationAngle().ToFloat(),
              _initialCubePose.GetRotationAngle().getDegrees());
      
      ss << "InitialCubePose " << buf << "\n\t\t";
      
      Pose3d p;
      _initialCubePose.GetWithRespectTo(_initialRobotPose, p);
      sprintf(buf, "Translation=(%f, %f %f), RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg)",
              p.GetTranslation().x(),
              p.GetTranslation().y(),
              p.GetTranslation().z(),
              p.GetRotationAxis().x(),
              p.GetRotationAxis().y(),
              p.GetRotationAxis().z(),
              p.GetRotationAngle().ToFloat(),
              p.GetRotationAngle().getDegrees());
      
      ss << "CubeRelativeToRobot " << buf << "\n";
    
      // For log parsing
      // result, rel block pose x, rel block pose y, rel block angle, dock method, didHM, numDockingRetries
      // numSawObject
      sprintf(buf, "!!!%i %f %f %f %i %i %i %i",
              (int)result,
              p.GetTranslation().x(),
              p.GetTranslation().y(),
              p.GetRotationAngle().ToFloat(),
              (int)kTestDockingMethod,
              _didHM,
              _numDockingRetries,
              _numSawObject);
      ss << buf << "\n";
      
      Write(ss.str());
    }
    
    void BehaviorDockingTestSimple::Write(const std::string& s)
    {
      _logger.Write(s + "\n");
    }
    
    void BehaviorDockingTestSimple::PrintStats()
    {
      // If we aren't stopping cleanly then don't count this run as an attempt
      if(_currentState != State::ManualReset)
      {
        std::stringstream ss;
        ss << "Attempt: " << _numAttempts << " Stopped while running";
        Write(ss.str());
        _numAttempts--;
      }
      
      std::stringstream ss;
      ss << "*****************\n";
      ss << "Successful Runs: " << _numAttempts-_numFails+_numExtraAttemptsDueToFailure << " (";
      ss << ((_numAttempts-_numFails+_numExtraAttemptsDueToFailure)/(1.0*_numAttempts+_numExtraAttemptsDueToFailure))*100 << "%)\n";
      ss << "Failed Runs: " << _numFails << " (" << (_numFails/(1.0*_numAttempts+_numExtraAttemptsDueToFailure))*100 << "%)\n";
      ss << "Total Runs: " << _numAttempts+_numExtraAttemptsDueToFailure;
      Write(ss.str());
      Write("=====End DockingTestSimple=====");
    }
  }
}