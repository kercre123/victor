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

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDockingTestSimple.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/robotToEngineImplMessaging.h"
#include "clad/types/dockingSignals.h"
#include "clad/types/pathMotionProfile.h"
#include "util/console/consoleInterface.h"
#include <ctime>

#define DEBUG_DOCKING_TEST_BEHAVIOR 1

#define END_TEST_IN_HANDLER(RESULT, NAME) EndAttempt(robot, RESULT, NAME); return;

namespace Anki {
namespace Cozmo {

namespace{
// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
} while(0) \

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
static const f32 kInvalidAngle = 1000;

static const BackpackLightAnimation::BackpackAnimation passLights = {
  .onColors               = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{1000,1000,1000}},
  .offPeriod_ms           = {{100,100,100}},
  .transitionOnPeriod_ms  = {{450,450,450}},
  .transitionOffPeriod_ms = {{450,450,450}},
  .offset                 = {{0,0,0}}
};

static const BackpackLightAnimation::BackpackAnimation failLights = {
  .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{500,500,500}},
  .offPeriod_ms           = {{500,500,500}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDockingTestSimple::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDockingTestSimple::DynamicVariables::DynamicVariables()
: initialVisionMarker(Vision::MARKER_UNKNOWN)
, markerBeingSeen(Vision::MARKER_UNKNOWN)
{
  currentState = State::Init;

  initialPreActionPoseAngle_rad = kInvalidAngle;
  reset                  = false;
  yellForHelp            = false;
  yellForCompletion      = false;
  endingFromFailedAction = false;
  numSawObject           = 0;

  // Stats for test/attempts
  numFails                     = 0;
  numDockingRetries            = 0;
  numAttempts                  = 0;
  numExtraAttemptsDueToFailure = 0;
  numConsecFails               = 0;
  didHM                        = false;
  failedCurrentAttempt         = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDockingTestSimple::BehaviorDockingTestSimple(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({{
    EngineToGameTag::RobotCompletedAction,
    EngineToGameTag::RobotObservedObject,        
    EngineToGameTag::ObjectMoved,
    EngineToGameTag::RobotStopped,
    EngineToGameTag::RobotOffTreadsStateChanged
  }});

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::InitBehavior()
{
  _iConfig.logger = std::make_unique<Util::RollingFileLogger>(nullptr, 
        GetBEI().GetRobotInfo()._robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Cache, "dockingTest"));

  const Robot& robot = GetBEI().GetRobotInfo()._robot;
  if(nullptr != robot.GetContext()->GetRobotManager() &&
      robot.GetContext()->GetRobotManager()->GetMsgHandler() != nullptr)
  {
    _iConfig.signalHandle = (robot.GetContext()->GetRobotManager()->GetMsgHandler()->Subscribe(RobotInterface::RobotToEngineTag::dockingStatus,
                                                                                        std::bind(&BehaviorDockingTestSimple::HandleDockingStatus, this, std::placeholders::_1)));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDockingTestSimple::WantsToBeActivatedBehavior() const
{
  return _dVars.currentState == State::Init;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::OnBehaviorActivated()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  _dVars.cubePlacementPose = Pose3d(Radians(DEG_TO_RAD(0)), Z_AXIS_3D(), {176, 0, 22}, robot.GetWorldOrigin());

  // force the default speeds
  PathMotionProfile motionProfile;
  motionProfile.isCustom = true;
  SmartSetMotionProfile(motionProfile);
  
  _dVars.currentState = State::Init;
  _dVars.numFails = 0;
  _dVars.numDockingRetries = 0;
  _dVars.numAttempts = 0;
  _dVars.numConsecFails = 0;
  _dVars.didHM = false;
  _dVars.failedCurrentAttempt = false;
  
  _dVars.actionCallbackMap.clear();
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
    _iConfig.imageFolder = ss.str();
    robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Stream, ss.str());
  }      
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::BehaviorUpdate()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;
  if(_dVars.numAttempts == kMaxNumAttempts && _dVars.currentState != State::ManualReset)
  {
    Write("\nTest Completed Successfully");
    
    PrintStats();
    
    if(robot.GetContext()->GetVizManager() != nullptr)
    {
      robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Stream);
    }
    
    _dVars.yellForCompletion = true;
    SetCurrStateAndFlashLights(State::ManualReset, robot);
  }
  
  if(IsControlDelegated())
  {
    return;
  }
  
  DEV_ASSERT(robot.IsPoseInWorldOrigin(robot.GetPose()),
              "BehaviorDockingTestSimple.UpdateInternal_Legacy.BadRobotPoseOrigin");
  
  switch(_dVars.currentState)
  {
    case State::Init:
    {
      // Cancel all actions
      for (const auto& tag : _dVars.actionCallbackMap) {
        robot.GetActionList().Cancel(tag.first);
      }
      _dVars.actionCallbackMap.clear();
      
      if(robot.GetContext()->GetVizManager() != nullptr)
      {
        robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Stream, _iConfig.imageFolder);
      }
      
      // Turn off backpack lights in case we needed to be manually reset
      robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);
      
      _dVars.blockObjectIDPickup.UnSet();
      
      _dVars.cubePlacementPose = Pose3d(Radians(DEG_TO_RAD(0)), Z_AXIS_3D(), {176, 0, 22}, robot.GetWorldOrigin());
      
      _dVars.initialPreActionPoseAngle_rad = kInvalidAngle;
      
      CompoundActionSequential* action = new CompoundActionSequential({new MoveHeadToAngleAction(0, DEG_TO_RAD(1), 0), new WaitAction(2)});
      DelegateIfInControl(robot, action,
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
      _dVars.numAttempts++;
      _dVars.didHM = false;
      _dVars.numDockingRetries = 0;
      _dVars.failedCurrentAttempt = false;
      _dVars.attemptStartTime = robot.GetRobotState().lastImageTimeStamp;
      
      if(kRollInsteadOfPickup)
      {
        SetCurrState(State::Roll);
      }
      else
      {
        SetCurrState(State::PickupLow);
      }
      _dVars.numSawObject = 0;
      break;
    }
    case State::Roll:
    {
      if(_dVars.blockObjectIDPickup.IsSet())
      {
        ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_dVars.blockObjectIDPickup);
        if(nullptr == object)
        {
          EndAttempt(robot, ActionResult::ABORT, "PickupObjectIsNull", true);
          CancelSelf();
          return;
        }
        
        _dVars.initialCubePose = object->GetPose();
        _dVars.initialRobotPose = robot.GetPose();
        
        Block* block = dynamic_cast<Block*>(object);
        Pose3d junk;
        _dVars.initialVisionMarker = const_cast<Vision::KnownMarker&>(block->GetTopMarker(junk));
        
        
        DriveToObjectAction* driveAction = new DriveToObjectAction(_dVars.blockObjectIDPickup, PreActionPose::ROLLING);
        DelegateIfInControl(robot, driveAction,
                    [this, &robot](const ActionResult& result, const ActionCompletedUnion& completionUnion){
                      if(result == ActionResult::SUCCESS)
                      {
                        _dVars.initialRobotPose = robot.GetPose();
                        
                        RollObjectAction* action = new RollObjectAction(_dVars.blockObjectIDPickup);
                        action->SetDockingMethod((DockingMethod)kTestDockingMethod);
                        action->EnableDeepRoll(kDoDeepRoll);
                        
                        DelegateIfInControl(robot, action,
                                    [this, &robot](const ActionResult& result, const ActionCompletedUnion& completedUnion){
                                      if (result != ActionResult::SUCCESS) {
                                        if(_dVars.numSawObject < 5)
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
        _dVars.yellForHelp = true;
        SetCurrStateAndFlashLights(State::ManualReset, robot);
      }
      break;
    }
    case State::PickupLow:
    {
      if(_dVars.blockObjectIDPickup.IsSet())
      {
        const ObservableObject* blockToPickup = robot.GetBlockWorld().GetLocatedObjectByID(_dVars.blockObjectIDPickup);
        if(nullptr == blockToPickup)
        {
          EndAttempt(robot, ActionResult::ABORT, "BlockToPickupNull", true);
          break;
        }
      
        const ActionableObject* aObject = dynamic_cast<const ActionableObject*>(blockToPickup);
        
        if(aObject == nullptr)
        {
          PRINT_NAMED_ERROR("BehaviorDockingTest.PickupLow.NullObject", "ActionableObject is null");
          CancelSelf();
          return;
        }
        
        // Get the preAction poses corresponding with _dVars.initialVisionMarker
        std::vector<PreActionPose> preActionPoses;
        std::vector<std::pair<Quad2f, ObjectID> > obstacles;
        robot.GetBlockWorld().GetObstacles(obstacles);
        aObject->GetCurrentPreActionPoses(preActionPoses,
                                          robot.GetPose(),
                                          {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                          {_dVars.initialVisionMarker.GetCode()},
                                          obstacles,
                                          nullptr);
        
        if(preActionPoses.empty())
        {
          break;
        }
        
        const Pose3d preActionPose = (kUseClosePreActionPose ? preActionPoses.back().GetPose() : preActionPoses.front().GetPose());
        
        // Store the angle of the preAction pose so we can use it as the approach angle for
        // the driveTo/dock actions so that we will always approach the object from the same angle
        if(_dVars.initialPreActionPoseAngle_rad == kInvalidAngle)
        {
          _dVars.initialPreActionPoseAngle_rad = preActionPose.GetRotation().GetAngleAroundZaxis().ToFloat();
        }
      
        // If we are adding random obstacles
        if(kNumRandomObstacles > 0)
        {
          // Delete the old obstacles we added
          BlockWorldFilter filter;
          filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
          filter.AddAllowedFamily(ObjectFamily::CustomObject);
          robot.GetBlockWorld().DeleteLocatedObjects(filter);
        
          // Add new obstacles at random poses around the preDock pose corresponding with _dVars.initialVisionMarker
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

            Pose3d p(angle, Z_AXIS_3D(), {x + randX, y + randY, kObstacleSize_mm.z() * 0.5f}, robot.GetWorldOrigin());
          
            robot.GetBlockWorld().CreateFixedCustomObject(p,
                                                          kObstacleSize_mm.x(),
                                                          kObstacleSize_mm.y(),
                                                          kObstacleSize_mm.z());
          }
        }
        
        _dVars.initialCubePose = blockToPickup->GetPose();
        _dVars.initialRobotPose = robot.GetPose();
        
        // If we are just doing a straight pickup without driving to the preDock pose
        if(kJustPickup)
        {
          PickupObjectAction* action = new PickupObjectAction(_dVars.blockObjectIDPickup);
          action->SetDockingMethod((DockingMethod)kTestDockingMethod);
          action->SetDoNearPredockPoseCheck(false);
          DelegateIfInControl(robot, action,
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        if (result != ActionResult::SUCCESS) {
                          if(_dVars.numSawObject < 5)
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
            action = new CompoundActionSequential(
              {new DriveToAlignWithObjectAction(_dVars.blockObjectIDPickup,
                                                0,
                                                false,
                                                0,
                                                AlignmentType::LIFT_PLATE),
                new MoveLiftToHeightAction(
                                          MoveLiftToHeightAction::Preset::CARRY)
              });
          }
          else
          {
            action = new DriveToPickupObjectAction(
                                                    _dVars.blockObjectIDPickup,
                                                    true,
                                                    _dVars.initialPreActionPoseAngle_rad);
            static_cast<DriveToPickupObjectAction*>(action)->SetDockingMethod((DockingMethod)kTestDockingMethod);
          }
          
          DelegateIfInControl(robot, action,
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        if (result != ActionResult::SUCCESS) {
                          if(_dVars.numSawObject < 5)
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
                          robot.GetCarryingComponent().SetCarryingObject(_dVars.blockObjectIDPickup, _dVars.markerBeingSeen.GetCode());
                        }
                        
                        SetCurrState(State::PlaceLow);
                        return true;
                      });
        }
        else
        {
          DriveToObjectAction* driveAction = new DriveToObjectAction(
                                                                      _dVars.blockObjectIDPickup,
                                                                      PreActionPose::DOCKING,
                                                                      0,
                                                                      true,
                                                                      _dVars.initialPreActionPoseAngle_rad);
          DelegateIfInControl(robot, driveAction,
                      [this, &robot](const ActionResult& result, const ActionCompletedUnion& completionUnion){
                        if(result == ActionResult::SUCCESS)
                        {
                          _dVars.initialRobotPose = robot.GetPose();
                          PickupObjectAction* action = new PickupObjectAction(_dVars.blockObjectIDPickup);
                          action->SetDockingMethod((DockingMethod)kTestDockingMethod);
                          DelegateIfInControl(robot, action,
                                      [this, &robot](const ActionResult& result, const ActionCompletedUnion& completedUnion){
                                        if (result != ActionResult::SUCCESS) {
                                          if(_dVars.numSawObject < 5)
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
        _dVars.yellForHelp = true;
        SetCurrStateAndFlashLights(State::ManualReset, robot);
      }
      break;
    }
    case State::PlaceLow:
    {
      PlaceObjectOnGroundAtPoseAction* action = new PlaceObjectOnGroundAtPoseAction(
                                                                                    _dVars.cubePlacementPose,
                                                                                    true);
      DelegateIfInControl(robot, action,
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
          CancelSelf();
          return;
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
      ActionableObject* aObject = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetLocatedObjectByID(_dVars.blockObjectIDPickup));
      
      if(aObject == nullptr)
      {
        PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.NullObject", "ActionableObject is null");
        CancelSelf();
        return;
      }
      
      std::vector<PreActionPose> preActionPoses;
      std::vector<std::pair<Quad2f, ObjectID> > obstacles;
      robot.GetBlockWorld().GetObstacles(obstacles);
      aObject->GetCurrentPreActionPoses(preActionPoses,
                                        robot.GetPose(),
                                        {(kRollInsteadOfPickup ? PreActionPose::ROLLING : PreActionPose::DOCKING)},
                                        {_dVars.initialVisionMarker.GetCode()},
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
                                            {_dVars.markerBeingSeen.GetCode()},
                                            obstacles,
                                            nullptr);
          
          if(preActionPoses.size() != kExpectedNumPreDockPoses)
          {
            PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.RollingBadNumPreActionPoses",
                              "Found %i preActionPoses for marker %s",
                              (int)preActionPoses.size(),
                              _dVars.markerBeingSeen.GetCodeName());
            CancelSelf();
            return;
          }
        }
        else
        {
          PRINT_NAMED_ERROR("BehaviorDockingTest.Reset.BadNumPreActionPoses",
                            "Found %i preActionPoses for marker %s",
                            (int)preActionPoses.size(),
                            _dVars.initialVisionMarker.GetCodeName());
          CancelSelf();
          return;
        }
      }
      
      // The closer preaction pose is the second on in the vector
      Pose3d preActionPose = (kUseClosePreActionPose ? preActionPoses.back().GetPose() : preActionPoses.front().GetPose());
      f32 x = preActionPose.GetTranslation().x();
      f32 y = preActionPose.GetTranslation().y();
      f32 angle = preActionPose.GetRotation().GetAngleAroundZaxis().ToFloat();
      
      // If we are reseting due to an action failing then we shouldn't go to a random offset
      // and we should not count this reset pickup as an attempt
      if(_dVars.endingFromFailedAction)
      {
        randX = 0;
        randY = 0;
        randA = 0;
        _dVars.endingFromFailedAction = false;
        _dVars.numAttempts--;
        _dVars.numExtraAttemptsDueToFailure++;
      }
      
      Pose3d p(Radians(angle + randA), Z_AXIS_3D(), {x + randX, y + randY, 0}, robot.GetWorldOrigin());
      
      ICompoundAction* action = new CompoundActionSequential();
      
      // If we are carrying the object when we reset then make sure to put it down before driving to the pose
      if(robot.GetCarryingComponent().IsCarryingObject())
      {
        PlaceObjectOnGroundAtPoseAction* placeAction = new PlaceObjectOnGroundAtPoseAction(
                                                                                            _dVars.cubePlacementPose,
                                                                                            true);
        action->AddAction(placeAction);
      }
      
      const bool kDriveWithDown = true;
      DriveToPoseAction* driveAction = new DriveToPoseAction(p, kDriveWithDown);
      action->AddAction(driveAction);
      
      DelegateIfInControl(robot, action,
                  [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                    if (result != ActionResult::SUCCESS) {
                      EndAttempt(robot, result, "DriveToPose", true);
                      return false;
                    }
                    SetCurrState(State::Inactive);
                    if(!_dVars.failedCurrentAttempt)
                    {
                      RecordAttempt(result, "");
                      _dVars.numConsecFails = 0;
                    }
                    return true;
                  });
      break;
    }
    case State::ManualReset:
    {
      _dVars.reset = true;
      
      if(_dVars.yellForCompletion)
      {
        _dVars.yellForCompletion = false;
        
        if(robot.GetContext()->GetVizManager() != nullptr)
        {
          robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Off);
        }
        
        IActionRunner* action = new CompoundActionSequential({new SayTextAction("Test Complete", SayTextAction::AudioTtsProcessingStyle::Unprocessed),
                                                              new WaitAction(3)});
        DelegateIfInControl(robot, action,
                    [this](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                      if(result == ActionResult::SUCCESS)
                      {
                        _dVars.yellForCompletion = true;
                      }
                      return true;
                    });
      }
      else if(_dVars.yellForHelp)
      {
        _dVars.yellForHelp = false;
        
        if(robot.GetContext()->GetVizManager() != nullptr)
        {
          robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Off);
        }
        
        IActionRunner* action = new CompoundActionSequential({new SayTextAction("Help", SayTextAction::AudioTtsProcessingStyle::Unprocessed),
                                                              new WaitAction(3)});
        DelegateIfInControl(robot, action,
                    [this](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                      if(result == ActionResult::SUCCESS)
                      {
                        _dVars.yellForHelp = true;
                      }
                      return true;
                    });
      }
      
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorDockingTest.Update.UnknownState", "Reached unknown state %d", (u32)_dVars.currentState);
      CancelSelf();
      return;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::OnBehaviorDeactivated()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Cancel all actions
  for (const auto& tag : _dVars.actionCallbackMap) {
    robot.GetActionList().Cancel(tag.first);
  }
  _dVars.actionCallbackMap.clear();
  
  PrintStats();
  
  if(robot.GetContext()->GetVizManager() != nullptr)
  {
    robot.GetContext()->GetVizManager()->SendSaveImages(ImageSendMode::Off);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::SetCurrState(State s)
{
  _dVars.currentState = s;
  
  UpdateStateName();
  
  BEHAVIOR_VERBOSE_PRINT(DEBUG_DOCKING_TEST_BEHAVIOR, "BehaviorDockingTest.SetState",
                          "set state to '%s'", GetDebugStateName().c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::SetCurrStateAndFlashLights(State s, Robot& robot)
{
  if(_dVars.yellForHelp)
  {
    robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);
  }
  else if(_dVars.yellForCompletion)
  {
    robot.GetBackpackLightComponent().SetBackpackAnimation(passLights);
  }
  SetCurrState(s);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::UpdateStateName()
{
  std::string name = StateToString(_dVars.currentState);
  
  name += std::to_string(_dVars.numAttempts+_dVars.numExtraAttemptsDueToFailure);
  
  if(_dVars.currentState == State::Reset && _dVars.endingFromFailedAction)
  {
    name += "FromFailure";
  }
  
  if( IsControlDelegated() ) {
    name += '*';
  }
  else {
    name += ' ';
  }
  
  SetDebugStateName(name);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::HandleWhileActivated(const EngineToGameEvent& event)
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;
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
        _dVars.currentState = State::Init;
        
        _dVars.numDockingRetries = 0;
        _dVars.numConsecFails = 0;
        _dVars.didHM = false;
        _dVars.failedCurrentAttempt = false;
        
        
        if(_dVars.reset)
        {
          Write("\n\n------Test Manually Reset------\n");
        }
        
        _dVars.yellForCompletion = false;
        _dVars.yellForHelp = false;
        _dVars.reset = false;
        
        _dVars.cubePlacementPose.SetParent(robot.GetWorldOrigin());
      }
      break;
    }
      
    default:
      PRINT_NAMED_INFO("BehaviorDockingTest.HandleWhileRunning.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::HandleActionCompleted(Robot& robot,
                                                      const ExternalInterface::RobotCompletedAction &msg)
{
  if(!IsActivated()) {
    // Ignore action completion messages while not running
    return;
  }
  
  auto callback = _dVars.actionCallbackMap.find(msg.idTag);
  
  if(callback == _dVars.actionCallbackMap.end())
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
    _dVars.actionCallbackMap.erase(callback);
  }
  
} // HandleActionCompleted()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::EndAttempt(Robot& robot, ActionResult result, std::string name, bool endingFromFailedAction)
{
  _dVars.endingFromFailedAction = endingFromFailedAction;
  RecordAttempt(result, name);
  SetCurrState(State::Reset);
  if(result != ActionResult::SUCCESS)
  {
    if(!_dVars.failedCurrentAttempt)
    {
      _dVars.numFails++;
    }
    _dVars.failedCurrentAttempt = true;
    if(++_dVars.numConsecFails >= kMaxConsecFails)
    {
      Write("\nReached max number of consecutive failed attempts");
      _dVars.yellForHelp = true;
      SetCurrStateAndFlashLights(State::ManualReset, robot);
    }
  }
  else
  {
    _dVars.numConsecFails = 0;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
    if(_dVars.blockObjectIDPickup.IsSet() && _dVars.blockObjectIDPickup == objectID)
    {
      _dVars.numSawObject++;
      
      std::vector<const Vision::KnownMarker*> observedMarkers;
      oObject->GetObservedMarkers(observedMarkers);
      if(observedMarkers.size() == 1)
      {
        _dVars.markerBeingSeen = observedMarkers.front()->GetCode();
      }
      else
      {
        PRINT_NAMED_INFO("BehaviorDockingTest.HandleObservedObject", "Saw more than one marker");
        _dVars.yellForHelp = true;
        END_TEST_IN_HANDLER(ActionResult::ABORT, "SawMoreThanOneMarkerOnInit");
      }
      return;
    }
    else if(!_dVars.blockObjectIDPickup.IsSet())
    {
      _dVars.blockObjectIDPickup = objectID;
      std::vector<const Vision::KnownMarker*> observedMarkers;
      oObject->GetObservedMarkers(observedMarkers);
      if(observedMarkers.size() == 1)
      {
        _dVars.initialVisionMarker = observedMarkers.front()->GetCode();
      }
      else
      {
        PRINT_NAMED_INFO("BehaviorDockingTest.HandleObservedObject", "Saw more than one marker");
        _dVars.yellForHelp = true;
        END_TEST_IN_HANDLER(ActionResult::ABORT, "SawMoreThanOneMarkerOnInit");
      }
      return;
    }
    else
    {
      PRINT_NAMED_WARNING("BehaviorDockingTest.HandleObservedObject.UnexpectedBlock", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
      _dVars.yellForHelp = true;
      END_TEST_IN_HANDLER(ActionResult::ABORT, "UnexpectedBlock");
    }
  }
  else
  {
    PRINT_NAMED_WARNING("BehaviorDockingTest.HandleObservedObject.UnexpectedObservedObject", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
    _dVars.yellForHelp = true;
    END_TEST_IN_HANDLER(ActionResult::ABORT, "UnexpectedObject");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg)
{
  EndAttempt(robot, ActionResult::ABORT, "RobotStopped");
  _dVars.yellForHelp = true;
  SetCurrStateAndFlashLights(State::ManualReset, robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::HandleDockingStatus(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const DockingStatus& payload = message.GetData().Get_dockingStatus();
  switch(payload.status)
  {
    case(Anki::Cozmo::Status::STATUS_BACKING_UP):
    {
      _dVars.numDockingRetries++;
      break;
    }
    case(Anki::Cozmo::Status::STATUS_DOING_HANNS_MANEUVER):
    {
      _dVars.didHM = true;
      break;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::DelegateIfInControl(Robot& robot, IActionRunner* action, ActionResultCallback callback)
{
  assert(_dVars.actionCallbackMap.count(action->GetTag()) == 0);
  
  if (robot.GetActionList().QueueAction(QueueActionPosition::NOW, action) == RESULT_OK) {
    _dVars.actionCallbackMap[action->GetTag()] = callback;
  } else {
    PRINT_NAMED_WARNING("BehaviorDockingTest.DelegateIfInControl.QueueActionFailed", "Action type %s", EnumToString(action->GetType()));
    EndAttempt(robot, ActionResult::ABORT, "QueueActionFailed");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::RecordAttempt(ActionResult result, std::string name)
{
  std::stringstream ss;
  ss << "Attempt: " << _dVars.numAttempts+_dVars.numExtraAttemptsDueToFailure << " TimeStarted: " << _dVars.attemptStartTime << " Result: " << EnumToString(result);
  ss << " " << name << " DockingMethod: " << (int)kTestDockingMethod;
  ss << " HM: " << _dVars.didHM << " NumDockingRetries: " << _dVars.numDockingRetries << "\n\t\t";
  
  char buf[300];
  sprintf(buf, "Translation=(%f, %f %f), RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg)",
          _dVars.initialRobotPose.GetTranslation().x(),
          _dVars.initialRobotPose.GetTranslation().y(),
          _dVars.initialRobotPose.GetTranslation().z(),
          _dVars.initialRobotPose.GetRotationAxis().x(),
          _dVars.initialRobotPose.GetRotationAxis().y(),
          _dVars.initialRobotPose.GetRotationAxis().z(),
          _dVars.initialRobotPose.GetRotationAngle().ToFloat(),
          _dVars.initialRobotPose.GetRotationAngle().getDegrees());
  
  ss << "InitialRobotPose " << buf << "\n\t\t";
  
  sprintf(buf, "Translation=(%f, %f %f), RotVec=(%f, %f, %f), RotAng=%frad (%.1fdeg)",
          _dVars.initialCubePose.GetTranslation().x(),
          _dVars.initialCubePose.GetTranslation().y(),
          _dVars.initialCubePose.GetTranslation().z(),
          _dVars.initialCubePose.GetRotationAxis().x(),
          _dVars.initialCubePose.GetRotationAxis().y(),
          _dVars.initialCubePose.GetRotationAxis().z(),
          _dVars.initialCubePose.GetRotationAngle().ToFloat(),
          _dVars.initialCubePose.GetRotationAngle().getDegrees());
  
  ss << "InitialCubePose " << buf << "\n\t\t";
  
  Pose3d p;
  _dVars.initialCubePose.GetWithRespectTo(_dVars.initialRobotPose, p);
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
          _dVars.didHM,
          _dVars.numDockingRetries,
          _dVars.numSawObject);
  ss << buf << "\n";
  
  Write(ss.str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::Write(const std::string& s)
{
  if(_iConfig.logger != nullptr){
    _iConfig.logger->Write(s + "\n");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDockingTestSimple::PrintStats()
{
  // If we aren't stopping cleanly then don't count this run as an attempt
  if(_dVars.currentState != State::ManualReset)
  {
    std::stringstream ss;
    ss << "Attempt: " << _dVars.numAttempts << " Stopped while running";
    Write(ss.str());
    _dVars.numAttempts--;
  }
  
  std::stringstream ss;
  ss << "*****************\n";
  ss << "Successful Runs: " << _dVars.numAttempts-_dVars.numFails+_dVars.numExtraAttemptsDueToFailure << " (";
  ss << ((_dVars.numAttempts-_dVars.numFails+_dVars.numExtraAttemptsDueToFailure)/(1.0*_dVars.numAttempts+_dVars.numExtraAttemptsDueToFailure))*100 << "%)\n";
  ss << "Failed Runs: " << _dVars.numFails << " (" << (_dVars.numFails/(1.0*_dVars.numAttempts+_dVars.numExtraAttemptsDueToFailure))*100 << "%)\n";
  ss << "Total Runs: " << _dVars.numAttempts+_dVars.numExtraAttemptsDueToFailure;
  Write(ss.str());
  Write("=====End DockingTestSimple=====");
}

} // namespace Cozmo
} // namespace Anki
