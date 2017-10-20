/**
 * File: behaviorPlaypenDriftCheck.cpp
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, speaker works, mics work, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenDistanceSensor.h"

#include "engine/actions/basicActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/proxSensorComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
static const std::string kAngleToTurnKey    = "AngleToTurnToSeeTarget_deg";
static const std::string kExpectedObjectKey = "ExpectedObjectType";
static const std::string kExpectedDistKey   = "ExpectedDistance_mm";
}

BehaviorPlaypenDistanceSensor::BehaviorPlaypenDistanceSensor(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  bool res = false;
  float angle = 0;
  res = JsonTools::GetValueOptional(config, kAngleToTurnKey, angle);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kAngleToTurnKey.c_str());
  }
  _angleToTurn = DEG_TO_RAD(angle);
  
  std::string objectType = "";
  res = JsonTools::GetValueOptional(config, kExpectedObjectKey, objectType);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kExpectedObjectKey.c_str());
  }
  _expectedObjectType = ObjectTypeFromString(objectType);
  
  res = JsonTools::GetValueOptional(config, kExpectedDistKey, _expectedDistanceToObject_mm);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kExpectedDistKey.c_str());
  }
  
  SubscribeToTags({EngineToGameTag::RobotObservedObject});
}

Result BehaviorPlaypenDistanceSensor::InternalInitInternal(Robot& robot)
{
  if(_expectedObjectType == ObjectType::UnknownObject)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.InternalInitInternal.NoObject",
                        "No object type specified to look for, failing");
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CUBE_NOT_FOUND, RESULT_FAIL);
  }

  _startingAngle = robot.GetPose().GetRotation().GetAngleAroundZaxis();

  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(robot, DEG_TO_RAD(0));
  MoveLiftToHeightAction* lift = new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK);
  CompoundActionParallel* liftAndHead = new CompoundActionParallel(robot, {lift, head});
  
  TurnInPlaceAction* turn = new TurnInPlaceAction(robot, _angleToTurn.ToFloat(), false);
  WaitForImagesAction* wait = new WaitForImagesAction(robot, 5);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {liftAndHead, turn, wait});
  StartActing(action, [this, &robot]() { TransitionToRefineTurn(robot); });
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenDistanceSensor::InternalUpdateInternal(Robot& robot)
{
  if(_numRecordedReadingsLeft == -1)
  {
    return BehaviorStatus::Running;
  }
  else if(_numRecordedReadingsLeft > 0)
  {
    --_numRecordedReadingsLeft;
    
    const auto& proxData = robot.GetProxSensorComponent().GetLatestProxData();
    DistanceSensorData data;
    data.proxSensorData = proxData;
    data.visualDistanceToTarget_mm = 0;
    data.visualAngleAwayFromTarget_rad = 0;
    
    Pose3d markerPose;
    const bool res = GetExpectedObjectMarkerPoseWrtRobot(robot, markerPose);
    if(res)
    {
      data.visualDistanceToTarget_mm = markerPose.GetTranslation().x();
      data.visualAngleAwayFromTarget_rad = (robot.GetPose().GetRotation().GetAngleAroundZaxis() -
                                            markerPose.GetRotation().GetAngleAroundZaxis() -
                                            DEG_TO_RAD(180.f)).ToFloat();
    }
    
    if(!GetLogger().Append(GetIDStr(), std::move(data)))
    {
      PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::WRITE_TO_LOG_FAILED, BehaviorStatus::Running);
    }
    
    return BehaviorStatus::Running;
  }
  else
  {
    if(!IsActing())
    {
      TransitionToTurnBack(robot);
    }
    return BehaviorStatus::Running;
  }
}

void BehaviorPlaypenDistanceSensor::StopInternal(Robot& robot)
{
  _startingAngle = 0;
  _numRecordedReadingsLeft = -1;
}

void BehaviorPlaypenDistanceSensor::TransitionToRefineTurn(Robot& robot)
{
  TurnInPlaceAction* action = new TurnInPlaceAction(robot, 0, false);

  BlockWorldFilter filter;
  filter.AddAllowedType(_expectedObjectType);

  Pose3d markerPose;
  const bool res = GetExpectedObjectMarkerPoseWrtRobot(robot, markerPose);
  if(res)
  {
    Radians angle = 0;
    const f32 distToMarker_mm = markerPose.GetTranslation().x();
    if(!Util::IsNear(distToMarker_mm,
                     _expectedDistanceToObject_mm,
                     PlaypenConfig::kVisualDistanceToDistanceSensorObjectThresh_mm))
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.TransitionToRefineTurn.MarkerOOR",
                          "Observing distance sensor marker at %fmm, expected distance is %fmm +/- %fmm",
                          distToMarker_mm,
                          _expectedDistanceToObject_mm,
                          PlaypenConfig::kVisualDistanceToDistanceSensorObjectThresh_mm);
      
      PLAYPEN_SET_RESULT(FactoryTestResultCode::DISTANCE_MARKER_OOR);
    }
    else
    {
      angle = -1*(robot.GetPose().GetRotation().GetAngleAroundZaxis() -
                  markerPose.GetRotation().GetAngleAroundZaxis());
    }
    
    PRINT_NAMED_INFO("BehaviorPlaypenDistanceSensor.TransitionToRefineTurn.TurnAngle",
                     "Turning %f degrees to be perpendicular to marker",
                     angle.getDegrees());
    
    action->SetRequestedTurnAngle(angle.ToFloat());
  }
  
  StartActing(action, [this, &robot]() { TransitionToRecordSensor(robot); });
}

void BehaviorPlaypenDistanceSensor::TransitionToRecordSensor(Robot& robot)
{
  _numRecordedReadingsLeft = PlaypenConfig::kNumDistanceSensorReadingsToRecord;
}

void BehaviorPlaypenDistanceSensor::TransitionToTurnBack(Robot& robot)
{
  TurnInPlaceAction* action = new TurnInPlaceAction(robot, _startingAngle.ToFloat(), true);
  StartActing(action, [this]() { PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS); });
}

bool BehaviorPlaypenDistanceSensor::GetExpectedObjectMarkerPoseWrtRobot(Robot& robot,
                                                                        Pose3d& markerPoseWrtRobot)
{
  BlockWorldFilter filter;
  filter.AddAllowedType(_expectedObjectType);
  ObservableObject* object = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  if(object == nullptr)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.GetExpectedObjectMarkerPoseWrtRobot.NullObject",
                        "Expected object of type %s does not exist or am not seeing it",
                        ObjectTypeToString(_expectedObjectType));
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
    return false;
  }
  else
  {
    const auto& markers = object->GetMarkers();
    
    Pose3d markerPose;
    TimeStamp_t lastObservedTime = 0;
    std::string markerName = "";
    for(const auto& marker : markers)
    {
      if(marker.GetLastObservedTime() > lastObservedTime)
      {
        lastObservedTime = marker.GetLastObservedTime();
        markerPose = marker.GetPose();
        markerName = marker.GetCodeName();
      }
    }
    
    const bool res = markerPose.GetWithRespectTo(robot.GetPose(), markerPoseWrtRobot);
    if(!res)
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.GetExpectedObjectMarkerPoseWrtRobot.NoClosestMarker",
                          "Failed to get closest marker pose for object %s with %d",
                          ObjectTypeToString(_expectedObjectType),
                          res);
      PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
      return false;
    }
    
    return true;
  }
}

}
}


