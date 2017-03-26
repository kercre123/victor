/**
 * File: driveToHelper.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/21/17
 *
 * Description: Handles driving to objects and searching for them if they aren't
 * visually verified
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/driveToHelper.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"


namespace Anki {
namespace Cozmo {  

namespace{
static const int kMaxNumRetrys = 3;
static const f32 driveBackDist_mm = 20;
static const f32 driveBackSpeed_mmps = 20;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DriveToHelper::DriveToHelper(Robot& robot,
                                   IBehavior& behavior,
                                   BehaviorHelperFactory& helperFactory,
                                   const ObjectID& targetID,
                                   const DriveToParameters& params)
: IHelper("DriveToHelper", robot, behavior, helperFactory)
, _targetID(targetID)
, _params(params)
, _tmpRetryCounter(0)
, _searchLevel(0)
, _searchStarted(false)
, _searchShouldEnd(false)
{
  const bool invalidPlaceRelParams =
                (_params.actionType != PreActionPose::PLACE_RELATIVE) &&
                 !(FLT_NEAR(_params.placeRelOffsetY_mm, 0.f) &&
                   FLT_NEAR(_params.placeRelOffsetX_mm, 0.f));
  
  
  auto observedObjectCallback =
  [this, &robot](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event){
    if(_searchStarted){
      const ObjectID objSeen = event.GetData().Get_RobotObservedObject().objectID;
      // Search should stop if we either 1) see the cube we're looking for and have
      // therefore updated its known pose or 2) see an object that may have been
      // obstructing the robots view without its knowledge previously
      if(objSeen == _targetID){
        _searchShouldEnd = true;
      }else if(_objectsSeenDuringSearch.find(objSeen) == _objectsSeenDuringSearch.end()){
        if(!ShouldBeAbleToFindTarget(robot)){
          _searchShouldEnd = true;
        }
        _objectsSeenDuringSearch.insert(objSeen);
      }
    }
  };
  
  if(robot.HasExternalInterface()){
    using namespace ExternalInterface;
    _eventHalders.push_back(robot.GetExternalInterface()->Subscribe(
                    ExternalInterface::MessageEngineToGameTag::RobotObservedObject,
                    observedObjectCallback));
  }
  
  
  DEV_ASSERT(!invalidPlaceRelParams,"DriveToHelper.InvalidPlaceRelParams");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DriveToHelper::~DriveToHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DriveToHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus DriveToHelper::Init(Robot& robot)
{
  _tmpRetryCounter = 0;
  _searchStarted = false;
  ResetSearchVariables(robot);
  DriveToPreActionPose(robot);
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus DriveToHelper::UpdateWhileActiveInternal(Robot& robot)
{
  if(_searchShouldEnd){
    StopActing(true);
    _status = BehaviorStatus::Complete;
  }
  
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::DriveToPreActionPose(Robot& robot)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    _status = BehaviorStatus::Failure;
    return;
  }
  _tmpRetryCounter++;
  
  if((_params.actionType != PreActionPose::PLACE_RELATIVE) ||
     ((_params.placeRelOffsetX_mm == 0) && (_params.placeRelOffsetY_mm == 0))){
    DriveToObjectAction* driveToAction = new DriveToObjectAction(robot,
                                                                 _targetID,
                                                                 _params.actionType);
    if(_params.useApproachAngle){
      driveToAction->SetApproachAngle(_params.approachAngle_rad);
    }
    StartActing(driveToAction, &DriveToHelper::RespondToDriveResult);
  }else{
    // Calculate the pre-dock pose directly for PLACE_RELATIVE and drive to that pose
    ActionableObject* obj = dynamic_cast<ActionableObject*>(
                           robot.GetBlockWorld().GetLocatedObjectByID(_targetID));
    if(obj != nullptr){
      std::vector<Pose3d> possiblePoses;
      bool alreadyInPosition;
      PlaceRelObjectAction::ComputePlaceRelObjectOffsetPoses(
                              obj, _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm,
                              robot, possiblePoses, alreadyInPosition);
      if(possiblePoses.size() > 0){
        if(alreadyInPosition){
          // Already in pose, no drive to necessary
          _status = BehaviorStatus::Complete;
        }else{
          // Drive to the nearest allowed pose, and then perform a visual verify
          CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
          compoundAction->AddAction(new DriveToPoseAction(robot, possiblePoses), true);
          compoundAction->AddAction(new VisuallyVerifyObjectAction(robot, _targetID));
          StartActing(compoundAction, &DriveToHelper::RespondToDriveResult);
        }
      }else{
        PRINT_NAMED_INFO("DriveToHelper.DriveToPreActionPose.NoPreDockPoses",
                         "No valid predock poses for objectID: %d with offsets x:%f y:%f",
                         _targetID.GetValue(),
                         _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm);
        robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
        _status = BehaviorStatus::Failure;
      }
    }else{
      PRINT_NAMED_WARNING("DriveToHelper.DriveToPreActionPose.TargetBlockNull",
                          "Failed to get ActionableObject for id:%d", _targetID.GetValue());
      _status = BehaviorStatus::Failure;
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::RespondToDriveResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = BehaviorStatus::Complete;
      break;
    }
    case ActionResult::VISUAL_OBSERVATION_FAILED:
    {
      _searchStarted = true;
      ResetSearchVariables(robot);
      SearchForBlock(result, robot);
      break;
    }
    case ActionResult::CANCELLED:
    {
      // leave the helper running, since it's about to be canceled
      break;
    }
    case ActionResult::NO_PREACTION_POSES:
    {
      robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
      break;
    }
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    case ActionResult::MOTOR_STOPPED_MAKING_PROGRESS:
    {
      DriveToPreActionPose(robot);
      break;
    }
    case ActionResult::NOT_STARTED:
    case ActionResult::BAD_OBJECT:
    case ActionResult::FAILED_TRAVERSING_PATH:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      DriveToPreActionPose(robot);
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::SearchForBlock(ActionResult result, Robot& robot)
{
  
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(staticBlock != nullptr){
    // Check if block observed during the last search
    if(_searchShouldEnd){
      _status = BehaviorStatus::Complete;
      return;
    }
    
    // Increment search level
    // Search for nearby - turn counter clockwise 90 deg - search for nearby
    // Turn clockwise 90 (from start) - search for nearby - nuclear option
    switch(_searchLevel){
      case 0:
      {
        auto searchNearby = new SearchForNearbyObjectAction(robot, _targetID);
        
        CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        compoundAction->AddAction(new TurnTowardsObjectAction(robot, _targetID, M_PI), true);
        compoundAction->AddAction(searchNearby);
        StartActing(compoundAction, &DriveToHelper::SearchForBlock);
        
        break;
      }
      case 2:
      case 4:
      {
        auto searchNearby = new SearchForNearbyObjectAction(robot, _targetID);
        StartActing(searchNearby, &DriveToHelper::SearchForBlock);
        break;
      }
      case 1:
      {
        CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        compoundAction->AddAction(new DriveStraightAction(robot,
                                                          -driveBackDist_mm,
                                                          driveBackSpeed_mmps,
                                                          false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, M_PI_4, false));

        StartActing(compoundAction, &DriveToHelper::SearchForBlock);
        break;
      }
      case 3:
      {
        CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        compoundAction->AddAction(new DriveStraightAction(robot,
                                                          -driveBackDist_mm,
                                                          driveBackSpeed_mmps,
                                                          false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_2, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false));
        
        StartActing(compoundAction, &DriveToHelper::SearchForBlock);
        break;
      }
      default:
      {
        if(staticBlock->IsPoseStateKnown()){
          PRINT_NAMED_ERROR("DriveToHelper.SearchForBlock.GoingNuclear",
                            "Failed to find known block - wiping");
          BlockWorldFilter filter;
          filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame); // not necessary, just to be explicit
          robot.GetBlockWorld().DeleteLocatedObjects(filter);
        }
        _status = BehaviorStatus::Failure;
      }
    }
  }else{
    _status = BehaviorStatus::Complete;
  }
  
  _searchLevel++;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DriveToHelper::ShouldBeAbleToFindTarget(Robot& robot)
{
  const ObservableObject* targetObj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(targetObj != nullptr){
    // check if the known object should no longer be visible given the robot's
    // camera while at the pre-dock pose
    static constexpr float kMaxNormalAngle = DEG_TO_RAD(45); // how steep of an angle we can see // ANDREW: is this true?
    static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it, size doesn't matter
    
    Vision::KnownMarker::NotVisibleReason reason =
    targetObj->IsVisibleFromWithReason(_robotCameraAtSearchStart,
                                       kMaxNormalAngle,
                                       kMinImageSizePix,
                                       false);
    
    return reason != Vision::KnownMarker::NotVisibleReason::OCCLUDED;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::ResetSearchVariables(Robot& robot)
{
  _searchLevel = 0;
  _searchShouldEnd = false;
  _robotCameraAtSearchStart = robot.GetVisionComponent().GetCamera();
  _objectsSeenDuringSearch.clear();
}

  
} // namespace Cozmo
} // namespace Anki

