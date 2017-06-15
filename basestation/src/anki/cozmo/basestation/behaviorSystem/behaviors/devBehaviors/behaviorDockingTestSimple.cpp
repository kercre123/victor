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
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorDockingTestSimple.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/components/carryingComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"
#include "clad/types/dockingSignals.h"
#include "util/console/consoleInterface.h"
#include <ctime>

#define DEBUG_DOCKING_TEST_BEHAVIOR 1

#define END_TEST_IN_HANDLER(RESULT, NAME) EndAttempt(robot, RESULT, NAME); return;

namespace{
// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
} while(0) \

static const char* kBehaviorTestName = "Docking test simple";
}



namespace Anki {
  namespace Cozmo {
    
    CONSOLE_VAR(u32, kMaxNumAttempts,              "DockingTest", 30);
    CONSOLE_VAR(u32, kMaxConsecFails,              "DockingTest", 3);
    CONSOLE_VAR(u32, kTestDockingMethod,           "DockingTest", (u8)DockingMethod::HYBRID_DOCKING);
    CONSOLE_VAR(f32, kMaxXAwayFromPreDock_mm,      "DockingTest", 50);
    CONSOLE_VAR(f32, kMaxYAwayFromPreDock_mm,      "DockingTest", 250);
    CONSOLE_VAR(f32, kMaxAngleAwayFromPreDock_deg, "DockingTest", 10);
    CONSOLE_VAR(bool, kDriveToAndPickupBlockOneAction, "DockingTest", true);
    CONSOLE_VAR(bool, kJustPickup,                 "DockingTest", false);
    CONSOLE_VAR(bool, kRollInsteadOfPickup,        "DockingTest", false);
    CONSOLE_VAR(bool, kAlignInsteadOfPickup,       "DockingTest", false);
    CONSOLE_VAR(bool, kDoDeepRoll,                 "DockingTest", false);
    CONSOLE_VAR(bool, kUseClosePreActionPose,      "DockingTest", false);
    CONSOLE_VAR(u32,  kNumRandomObstacles,         "DockingTest", 10);
    
    // Prevents obstacles from being created within +/- this y range of preDock pose
    // Ensures that no obstacles are created in front of the marker/preDock pose
    static const f32 kNoObstaclesWithinXmmOfPreDockPose = 40;
    
    // Obstacles will be created within +/- this range of preDock pose
    static const f32 kObstacleRangeY_mm = 300;
    
    // Obstacles will be created within this x range of the preDock pose
    // Negative is behind the preDock pose, positive is in front
    static const f32 kObstacleRangeXmin_mm = -200;
    static const f32 kObstacleRangeXmax_mm = 120;
    
    static const Point3f kObstacleSize_mm = {10, 10, 50};
    
    static const u32 kExpectedNumPreDockPoses = 1;
    
    static const BackpackLights passLights = {
      .onColors               = {{NamedColors::BLACK,NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN,NamedColors::BLACK}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{1000,1000,1000,1000,1000}},
      .offPeriod_ms           = {{100,100,100,100,100}},
      .transitionOnPeriod_ms  = {{450,450,450,450,450}},
      .transitionOffPeriod_ms = {{450,450,450,450,450}},
      .offset                 = {{0,0,0,0,0}}
    };
    
    static const BackpackLights failLights = {
      .onColors               = {{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{500,500,500,500,500}},
      .offPeriod_ms           = {{500,500,500,500,500}},
      .transitionOnPeriod_ms  = {{0,0,0,0,0}},
      .transitionOffPeriod_ms = {{0,0,0,0,0}},
      .offset                 = {{0,0,0,0,0}}
    };
    
    BehaviorDockingTestSimple::BehaviorDockingTestSimple(Robot& robot, const Json::Value& config)
    : IBehavior(robot, config)
    , _initialVisionMarker(Vision::MARKER_UNKNOWN)
    , _markerBeingSeen(Vision::MARKER_UNKNOWN)
    , _cubePlacementPose(Radians(DEG_TO_RAD(0)), Z_AXIS_3D(), {176, 0, 22}, &robot.GetPose().FindOrigin())
    , _logger(nullptr, robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, "dockingTest"))
    {
      
      _motionProfile = PathMotionProfile();
      _motionProfile.isCustom = true;
      
      SubscribeToTags({{
        EngineToGameTag::RobotCompletedAction,
        EngineToGameTag::RobotObservedObject,        
        EngineToGameTag::ObjectMoved,
        EngineToGameTag::RobotStopped,
        EngineToGameTag::RobotOffTreadsStateChanged
      }});
      
      if(nullptr != robot.GetContext()->GetRobotManager() &&
         robot.GetContext()->GetRobotManager()->GetMsgHandler() != nullptr)
      {
        _signalHandle = (robot.GetContext()->GetRobotManager()->GetMsgHandler()->Subscribe(robot.GetID(),
                                                                                           RobotInterface::RobotToEngineTag::dockingStatus,
                                                                                           std::bind(&BehaviorDockingTestSimple::HandleDockingStatus, this, std::placeholders::_1)));
      }
    }
    
    bool BehaviorDockingTestSimple::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
    {
      return _currentState == State::Init;
    }
    
    Result BehaviorDockingTestSimple::InitInternal(Robot& robot)
    {
      robot.GetBehaviorManager().DisableReactionsWithLock(kBehaviorTestName,
                                                         ReactionTriggerHelpers::kAffectAllArray);
      
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
      const RobotInterface::FWVersionInfo& fw = robot.GetRobotToEngineImplMessaging().GetFWVersionInfo();
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
          robot.GetBodyLightComponent().SetBackpackLights(failLights);
          
          _blockObjectIDPickup.UnSet();
          
          _cubePlacementPose = Pose3d(Radians(DEG_TO_RAD(0)), Z_AXIS_3D(), {176, 0, 22}, &robot.GetPose().FindOrigin());
          
          _initialPreActionPoseAngle_rad = kInvalidAngle;
          
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
            ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_blockObjectIDPickup);
            if(nullptr == object)
            {
              EndAttempt(robot, ActionResult::ABORT, "PickupObjectIsNull", true);
              return Status::Failure;
            }
            
            _initialCubePose = object->GetPose();
            _initialRobotPose = robot.GetPose();
            
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
            const ObservableObject* blockToPickup = robot.GetBlockWorld().GetLocatedObjectByID(_blockObjectIDPickup);
            if(nullptr == blockToPickup)
            {
              EndAttempt(robot, ActionResult::ABORT, "BlockToPickupNull", true);
              break;
            }
          
            const ActionableObject* aObject = dynamic_cast<const ActionableObject*>(blockToPickup);
            
            if(aObject == nullptr)
            {
              PRINT_NAMED_ERROR("BehaviorDockingTest.PickupLow.NullObject", "ActionableObject is null");
              return Status::Failure;
            }
            
            // Get the preAction poses corresponding with _initialVisionMarker
            std::vector<PreActionPose> preActionPoses;
            std::vector<std::pair<Quad2f, ObjectID> > obstacles;
            robot.GetBlockWorld().GetObstacles(obstacles);
            aObject->GetCurrentPreActionPoses(preActionPoses,
                                              robot.GetPose(),
                                              {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                              {_initialVisionMarker.GetCode()},
                                              obstacles,
                                              nullptr);
            
            if(preActionPoses.empty())
            {
              break;
            }
            
            const Pose3d preActionPose = (kUseClosePreActionPose ? preActionPoses.back().GetPose() : preActionPoses.front().GetPose());
            
            // Store the angle of the preAction pose so we can use it as the approach angle for
            // the driveTo/dock actions so that we will always approach the object from the same angle
            if(_initialPreActionPoseAngle_rad == kInvalidAngle)
            {
              _initialPreActionPoseAngle_rad = preActionPose.GetRotation().GetAngleAroundZaxis().ToFloat();
            }
          
            // If we are adding random obstacles
            if(kNumRandomObstacles > 0)
            {
              // Delete the old obstacles we added
              BlockWorldFilter filter;
              filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
              filter.AddAllowedFamily(ObjectFamily::CustomObject);
              robot.GetBlockWorld().DeleteLocatedObjects(filter);
            
              // Add new obstacles at random poses around the preDock pose corresponding with _initialVisionMarker
              for(u32 i = 0; i < kNumRandomObstacles; ++i)
              {
                f32 x = preActionPose.GetTranslation().x();
                f32 y = preActionPose.GetTranslation().y();
                Radians angle = preActionPose.GetRotation().GetAngleAroundZaxis();
                
                f32 randX = GetRNG().RandDblInRange(kObstacleRangeXmin_mm, kObstacleRangeXmax_mm);
                f32 randY = GetRNG().RandDblInRange(-kObstacleRangeY_mm, kObstacleRangeY_mm);
            
                // Keep picking a y value until it is outside the no obstacle zone
                int count = 0;
                while(randY < kNoObstaclesWithinXmmOfPreDockPose &&
                      randY > -kNoObstaclesWithinXmmOfPreDockPose)
                {
                  randY = GetRNG().RandDblInRange(-kObstacleRangeY_mm, kObstacleRangeY_mm);
                  count++;
                  
                  if(count > 100)
                  {
                    randY = kObstacleRangeY_mm;
                    break;
                  }
                }

                Pose3d p(angle, Z_AXIS_3D(), {x + randX, y + randY, kObstacleSize_mm.z() * 0.5f}, &robot.GetPose().FindOrigin());
              
                robot.GetBlockWorld().CreateFixedCustomObject(p,
                                                              kObstacleSize_mm.x(),
                                                              kObstacleSize_mm.y(),
                                                              kObstacleSize_mm.z());
              }
            }
            
            _initialCubePose = blockToPickup->GetPose();
            _initialRobotPose = robot.GetPose();
            
            // If we are just doing a straight pickup without driving to the preDock pose
            if(kJustPickup)
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
            // Otherwise if we are doing a DriveToPickupObject action in one state (instead of the driveTo and pickup
            // in separate states
            else if(kDriveToAndPickupBlockOneAction)
            {
              IActionRunner* action = nullptr;
              if(kAlignInsteadOfPickup)
              {
                // This compound action will essentially function as a DriveToPickupObjectAction
                // By aligning to the LIFT_PLATE and then moving the lift up we should
                // pickup the object
                action = new CompoundActionSequential(robot,
                  {new DriveToAlignWithObjectAction(robot,
                                                    _blockObjectIDPickup,
                                                    0,
                                                    false,
                                                    0,
                                                    AlignmentType::LIFT_PLATE),
                   new MoveLiftToHeightAction(robot,
                                              MoveLiftToHeightAction::Preset::CARRY)
                  });
              }
              else
              {
                action = new DriveToPickupObjectAction(robot,
                                                       _blockObjectIDPickup,
                                                       true,
                                                       _initialPreActionPoseAngle_rad);
                static_cast<DriveToPickupObjectAction*>(action)->SetDockingMethod((DockingMethod)kTestDockingMethod);
              }
              
              StartActing(robot, action,
                          [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                            if (result != ActionResult::SUCCESS) {
                              if(_numSawObject < 5)
                              {
                                EndAttempt(robot, result, "DriveToAndPickupNotSeeingObject", true);
                              }
                              else
                              {
                                EndAttempt(robot, result, "DriveToAndPickup", true);
                              }
                              return false;
                            }
                            if(kAlignInsteadOfPickup)
                            {
                              robot.GetCarryingComponent().SetCarryingObject(_blockObjectIDPickup, _markerBeingSeen.GetCode());
                            }
                            
                            SetCurrState(State::PlaceLow);
                            return true;
                          });
            }
            else
            {
              DriveToObjectAction* driveAction = new DriveToObjectAction(robot,
                                                                         _blockObjectIDPickup,
                                                                         PreActionPose::DOCKING,
                                                                         0,
                                                                         true,
                                                                          _initialPreActionPoseAngle_rad);
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
          PlaceObjectOnGroundAtPoseAction* action = new PlaceObjectOnGroundAtPoseAction(robot,
                                                                                        _cubePlacementPose,
                                                                                        true);
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
          
          if(kNumRandomObstacles > 0)
          {
            if(kMaxYAwayFromPreDock_mm <= kNoObstaclesWithinXmmOfPreDockPose)
            {
              PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.YAwayTooSmall",
                                "MaxYAwayFromPreDock_mm is too small %f <= %f need to be able to drive into obstacles",
                                kMaxYAwayFromPreDock_mm,
                                kNoObstaclesWithinXmmOfPreDockPose);
              return Status::Failure;
            }
            
            // Make sure y falls outside the no obstacle zone so we will always have to drive to a point
            // where obstacles could be
            int count = 0;
            while(randY < kNoObstaclesWithinXmmOfPreDockPose && randY > -kNoObstaclesWithinXmmOfPreDockPose)
            {
              randY = GetRNG().RandDblInRange(-kMaxYAwayFromPreDock_mm, kMaxYAwayFromPreDock_mm);
              count++;
              
              if(count > 100)
              {
                randY = kMaxYAwayFromPreDock_mm;
                break;
              }
            }
          }
          
          // Get the preActionPose relating to the marker we
          ActionableObject* aObject = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetLocatedObjectByID(_blockObjectIDPickup));
          
          if(aObject == nullptr)
          {
            PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.NullObject", "ActionableObject is null");
            return Status::Failure;
          }
          
          std::vector<PreActionPose> preActionPoses;
          std::vector<std::pair<Quad2f, ObjectID> > obstacles;
          robot.GetBlockWorld().GetObstacles(obstacles);
          aObject->GetCurrentPreActionPoses(preActionPoses,
                                            robot.GetPose(),
                                            {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                            {_initialVisionMarker.GetCode()},
                                            obstacles,
                                            nullptr);
          
          if(preActionPoses.size() != kExpectedNumPreDockPoses)
          {
            // If we are rolling and we can't find any preAction poses it means the block was not rolled
            // so get preActionPoses for the marker we are currently seeing
            if(kRollInsteadOfPickup)
            {
              aObject->GetCurrentPreActionPoses(preActionPoses,
                                                robot.GetPose(),
                                                {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                                {_markerBeingSeen.GetCode()},
                                                obstacles,
                                                nullptr);
              
              if(preActionPoses.size() != kExpectedNumPreDockPoses)
              {
                PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.RollingBadNumPreActionPoses",
                                  "Found %i preActionPoses for marker %s",
                                 (int)preActionPoses.size(),
                                 _markerBeingSeen.GetCodeName());
                return Status::Failure;
              }
            }
            else
            {
              PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.BadNumPreActionPoses",
                                "Found %i preActionPoses for marker %s",
                                (int)preActionPoses.size(),
                                _initialVisionMarker.GetCodeName());
              return Status::Failure;
            }
          }
          
          // The closer preaction pose is the second on in the vector
          Pose3d preActionPose = (kUseClosePreActionPose ? preActionPoses.back().GetPose() : preActionPoses.front().GetPose());
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
          
          ICompoundAction* action = new CompoundActionSequential(robot);
          
          // If we are carrying the object when we reset then make sure to put it down before driving to the pose
          if(robot.GetCarryingComponent().IsCarryingObject())
          {
            PlaceObjectOnGroundAtPoseAction* placeAction = new PlaceObjectOnGroundAtPoseAction(robot,
                                                                                               _cubePlacementPose,
                                                                                               true);
            placeAction->SetMotionProfile(_motionProfile);
            action->AddAction(placeAction);
          }
          
          const bool kDriveWithDown = true;
          DriveToPoseAction* driveAction = new DriveToPoseAction(robot, p, kDriveWithDown);
          driveAction->SetMotionProfile(_motionProfile);
          action->AddAction(driveAction);
          
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
            
            IActionRunner* action = new CompoundActionSequential(robot, {new SayTextAction(robot, "Test Complete", SayTextIntent::Text), new WaitAction(robot, 3)});
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
            
            IActionRunner* action = new CompoundActionSequential(robot, {new SayTextAction(robot, "Help", SayTextIntent::Text), new WaitAction(robot, 3)});
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
      robot.GetBehaviorManager().RemoveDisableReactionsLock(kBehaviorTestName);

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
                             "set state to '%s'", GetDebugStateName().c_str());
    }
    
    void BehaviorDockingTestSimple::SetCurrStateAndFlashLights(State s, Robot& robot)
    {
      if(_yellForHelp)
      {
        robot.GetBodyLightComponent().SetBackpackLights(failLights);
      }
      else if(_yellForCompletion)
      {
        robot.GetBodyLightComponent().SetBackpackLights(passLights);
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
      
      SetDebugStateName(name);
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
          
        case EngineToGameTag::RobotOffTreadsStateChanged:
        {
          if(event.GetData().Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads){
            _currentState = State::Init;
            
            _numDockingRetries = 0;
            _numConsecFails = 0;
            _didHM = false;
            _failedCurrentAttempt = false;
            
            
            if(_reset)
            {
              Write("\n\n------Test Manually Reset------\n");
            }
            
            _yellForCompletion = false;
            _yellForHelp = false;
            _reset = false;
            
            _cubePlacementPose.SetParent(robot.GetWorldOrigin());
          }
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
      const ObservableObject* oObject = robot.GetBlockWorld().GetLocatedObjectByID(objectID);
      
      if(nullptr == oObject)
      {
        PRINT_NAMED_WARNING("BehaviorDockingTestSimple.HandleObservedObject.NullObject",
                            "Object %d is NULL", objectID.GetValue());
        return;
      }
      
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
            END_TEST_IN_HANDLER(ActionResult::ABORT, "SawMoreThanOneMarkerOnInit");
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
            END_TEST_IN_HANDLER(ActionResult::ABORT, "SawMoreThanOneMarkerOnInit");
          }
          return;
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorDockingTest.HandleObservedObject.UnexpectedBlock", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          _yellForHelp = true;
          END_TEST_IN_HANDLER(ActionResult::ABORT, "UnexpectedBlock");
        }
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorDockingTest.HandleObservedObject.UnexpectedObservedObject", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
        _yellForHelp = true;
        END_TEST_IN_HANDLER(ActionResult::ABORT, "UnexpectedObject");
      }
    }
    
    void BehaviorDockingTestSimple::HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg)
    {
      EndAttempt(robot, ActionResult::ABORT, "RobotStopped");
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
      
      if (robot.GetActionList().QueueAction(QueueActionPosition::NOW, action) == RESULT_OK) {
        _actionCallbackMap[action->GetTag()] = callback;
      } else {
        PRINT_NAMED_WARNING("BehaviorDockingTest.StartActing.QueueActionFailed", "Action type %s", EnumToString(action->GetType()));
        EndAttempt(robot, ActionResult::ABORT, "QueueActionFailed");
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
