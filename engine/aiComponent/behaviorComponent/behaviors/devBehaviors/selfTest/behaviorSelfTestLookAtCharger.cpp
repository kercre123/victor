/**
 * File: behaviorSelfTestLookAtCharger.cpp
 *
 * Author: Al Chaussee
 * Created: 09/22/17
 *
 * Description: Turns towards a target marker and records a number of distance sensor readings
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTestLookAtCharger.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {

namespace {
static const char* kAngleToTurnKey    = "AngleToTurnToSeeTarget_deg";
static const char* kDistToDriveKey    = "DistanceToDriveToSeeTarget_mm";
static const char* kExpectedObjectKey = "ExpectedObjectType";
static const char* kExpectedDistKey   = "ExpectedDistance_mm";
}

BehaviorSelfTestLookAtCharger::BehaviorSelfTestLookAtCharger(const Json::Value& config)
: IBehaviorSelfTest(config, SelfTestResultCode::LOOK_AT_CHARGER_TIMEOUT)
{
  bool res = false;
  float angle = 0;
  res = JsonTools::GetValueOptional(config, kAngleToTurnKey, angle);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorSelfTestLookAtCharger.Constructor.MissingConfigKey",
                      "Missing %s key from SelfTestLookAtCharger Config", kAngleToTurnKey);
  }
  _angleToTurn = DEG_TO_RAD(angle);
  
  std::string objectType = "";
  res = JsonTools::GetValueOptional(config, kExpectedObjectKey, objectType);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorSelfTestLookAtCharger.Constructor.MissingConfigKey",
                      "Missing %s key from SelfTestLookAtCharger Config", kExpectedObjectKey);
  }
  _expectedObjectType = ObjectTypeFromString(objectType);
  
  res = JsonTools::GetValueOptional(config, kExpectedDistKey, _expectedDistanceToObject_mm);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorSelfTestLookAtCharger.Constructor.MissingConfigKey",
                      "Missing %s key from SelfTestLookAtCharger Config", kExpectedDistKey);
  }

  JsonTools::GetValueOptional(config, kDistToDriveKey, _distToDrive_mm);
  
  SubscribeToTags({EngineToGameTag::RobotObservedObject});
}

void BehaviorSelfTestLookAtCharger::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kAngleToTurnKey);
  expectedKeys.insert(kDistToDriveKey);
  expectedKeys.insert(kExpectedObjectKey);
  expectedKeys.insert(kExpectedDistKey);
}

Result BehaviorSelfTestLookAtCharger::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(_expectedObjectType == ObjectType::UnknownObject)
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.InternalInitInternal.NoObject",
                        "No object type specified to look for, failing");
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::CHARGER_NOT_FOUND, RESULT_FAIL);
  }

  // Record starting angle
  _startingAngle = robot.GetPose().GetRotation().GetAngleAroundZaxis();

  // Move head and lift to be able to see target marker and turn towards the target
  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(DEG_TO_RAD(0));
  MoveLiftToHeightAction* lift = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
  DriveStraightAction* drive = new DriveStraightAction(_distToDrive_mm, 60, false);
  CompoundActionParallel* liftHeadDrive = new CompoundActionParallel({lift, head, drive});

  TurnInPlaceAction* turn = new TurnInPlaceAction(_angleToTurn.ToFloat(), false);

  // After turning wait to process 10 images before trying to refine the turn
  WaitForImagesAction* wait = new WaitForImagesAction(5, VisionMode::Markers);
  
  CompoundActionSequential* action = new CompoundActionSequential({liftHeadDrive, turn, wait});
  DelegateIfInControl(action, [this]() { TransitionToRefineTurn(); });
  
  return RESULT_OK;
}

IBehaviorSelfTest::SelfTestStatus BehaviorSelfTestLookAtCharger::SelfTestUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Haven't started recording distance readings yet
  if(_numRecordedReadingsLeft == -1)
  {
    return SelfTestStatus::Running;
  }
  // We have distance readings left to record
  else if(_numRecordedReadingsLeft > 0)
  {
    --_numRecordedReadingsLeft;
    
    const auto& proxData = robot.GetProxSensorComponent().GetLatestProxData();
    DistanceSensorData data;
    data.proxDistanceToTarget_mm = proxData.distance_mm;
    data.visualDistanceToTarget_mm = 0;
    data.visualAngleAwayFromTarget_rad = 0;
    
    Pose3d markerPose;
    ObjectID unused;
    const bool res = GetExpectedObjectMarkerPoseWrtRobot(markerPose, unused);
    if(res)
    {
      data.visualDistanceToTarget_mm = markerPose.GetTranslation().x();

      markerPose = markerPose.GetWithRespectToRoot();
      // Marker pose rotation is kind of wonky, compared to the robot's rotation they are 
      // rotated 90 degrees. So when the robot is looking at a marker, you have to add
      // 90 degrees to get its rotation to match that of the robot
      // Taking the difference of these two angles tells us how much the robot needs to turn
      // to be perpendicular with the marker
      const auto angle = ((markerPose.GetRotation().GetAngleAroundZaxis() + DEG_TO_RAD(90)) - 
                          robot.GetPose().GetRotation().GetAngleAroundZaxis());
      data.visualAngleAwayFromTarget_rad = angle.ToFloat();
    }

    f32 bias = SelfTestConfig::kDistanceSensorBiasAdjustment_mm;
    if(!robot.IsPhysical())
    {
      bias = 0;
    }
    
    if(!Util::IsNear(data.proxDistanceToTarget_mm - bias, 
                     data.visualDistanceToTarget_mm,
                     SelfTestConfig::kDistanceSensorReadingThresh_mm))
    {
      PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.SelfTestUpdateInternal.ReadingOutsideThresh",
                          "Sensor reading %f - %f not near visual reading %f with threshold %f",
                          data.proxDistanceToTarget_mm,
                          SelfTestConfig::kDistanceSensorBiasAdjustment_mm,
                          data.visualDistanceToTarget_mm,
                          SelfTestConfig::kDistanceSensorReadingThresh_mm);

      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::DISTANCE_SENSOR_OOR, SelfTestStatus::Running);
    }
    
    return SelfTestStatus::Running;
  }
  // We've recorded all distance readings we need to
  else
  {
    // If we aren't acting then turn back to where we were facing when the behavior started
    if(!IsControlDelegated())
    {
      TransitionToTurnBack();
    }
    return SelfTestStatus::Running;
  }
}

void BehaviorSelfTestLookAtCharger::OnBehaviorDeactivated()
{
  _startingAngle = 0;
  _numRecordedReadingsLeft = -1;
}

void BehaviorSelfTestLookAtCharger::TransitionToRefineTurn()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  CompoundActionSequential* action = new CompoundActionSequential();
  TurnInPlaceAction* turn = new TurnInPlaceAction(0, false);

  // Get the pose of the marker we should be seeing
  Pose3d markerPose;
  ObjectID objectID;
  const bool res = GetExpectedObjectMarkerPoseWrtRobot(markerPose, objectID);
  if(res)
  {
    // Check that we are within expected distance to the marker
    Radians angle = 0;
    const f32 distToMarker_mm = markerPose.GetTranslation().x();
    if(!Util::IsNear(distToMarker_mm,
                     _expectedDistanceToObject_mm,
                     SelfTestConfig::kVisualDistanceToDistanceSensorObjectThresh_mm))
    {
      PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.TransitionToRefineTurn.MarkerOOR",
                          "Observing distance sensor marker at %fmm, expected distance is %fmm +/- %fmm",
                          distToMarker_mm,
                          _expectedDistanceToObject_mm,
                          SelfTestConfig::kVisualDistanceToDistanceSensorObjectThresh_mm);
      
      SELFTEST_SET_RESULT(SelfTestResultCode::DISTANCE_MARKER_VISUAL_OOR);
    }
    // We are within expected distance to update refined turn angle to put us perpendicular with the marker
    else
    {
      markerPose = markerPose.GetWithRespectToRoot();
      // Marker pose rotation is kind of wonky, compared to the robot's rotation they are 
      // rotated 90 degrees. So when the robot is looking at a marker, you have to add
      // 90 degrees to get its rotation to match that of the robot
      // Taking the difference of these two angles tells us how much the robot needs to turn
      // to be perpendicular with the marker
      angle = ((markerPose.GetRotation().GetAngleAroundZaxis() + DEG_TO_RAD(90)) - 
               robot.GetPose().GetRotation().GetAngleAroundZaxis());
    }
    
    PRINT_NAMED_INFO("BehaviorSelfTestLookAtCharger.TransitionToRefineTurn.TurnAngle",
                     "Turning %f degrees to be perpendicular to marker",
                     angle.getDegrees());
    
    turn->SetRequestedTurnAngle(angle.ToFloat());
  }
  action->AddAction(turn);

  VisuallyVerifyObjectAction* verify = new VisuallyVerifyObjectAction(objectID);
  verify->SetUseCyclingExposure();
  action->AddAction(verify);
  
  // Once we are perpendicular to the marker, start recording distance sensor readings
  DelegateIfInControl(action, [this]() { TransitionToRecordSensor(); });
}

void BehaviorSelfTestLookAtCharger::TransitionToRecordSensor()
{
  _numRecordedReadingsLeft = SelfTestConfig::kNumDistanceSensorReadingsToRecord;
}

void BehaviorSelfTestLookAtCharger::TransitionToTurnBack()
{
  SELFTEST_SET_RESULT(SelfTestResultCode::SUCCESS);
}

bool BehaviorSelfTestLookAtCharger::GetExpectedObjectMarkerPoseWrtRobot(Pose3d& markerPoseWrtRobot,
                                                                        ObjectID& objectID)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  BlockWorldFilter filter;
  filter.AddAllowedType(_expectedObjectType);
  std::vector<ObservableObject*> objects;
  robot.GetBlockWorld().FindLocatedMatchingObjects(filter, objects);

  if(objects.empty())
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.GetExpectedObjectMarkerPoseWrtRobot.NullObject",
                        "Expected object of type %s does not exist or am not seeing it",
                        ObjectTypeToString(_expectedObjectType));
    SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
    return false;
  }
  else
  {
    // Get the most recently observed object of the expected object type
    ObservableObject* object = nullptr;
    TimeStamp_t t = 0;
    for(const auto& obj : objects)
    {
      if(obj->GetLastObservedTime() > t)
      {
        object = obj;
        t = obj->GetLastObservedTime();
      }
    }

    if(object == nullptr)
    {
      PRINT_NAMED_INFO("BehaviorSelfTestLookAtCharger.GetExpectedObjectMarkerPoseWrtRobot.NullObject","");
      return false;
    }

    objectID = object->GetID();

    const auto& markers = object->GetMarkers();
    
    // Get the pose of the marker that was most recently observed
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

    if(lastObservedTime == 0)
    {
      PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.GetExpectedObjectMarkerPoseWrtRobot.MarkerNotObserved","");
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
      return false;
    }
    
    if((u32)robot.GetLastImageTimeStamp() - lastObservedTime > SelfTestConfig::kChargerMarkerLastObservedTimeThresh_ms)
    {
      PRINT_NAMED_INFO("BehaviorSelfTestLookAtCharger.GetExpectedObjectMarkerPoseWrtRobot.MarkerTooOld",
                       "%u - %u > %u",
                       (u32)robot.GetLastImageTimeStamp(),
                       lastObservedTime,
                       SelfTestConfig::kChargerMarkerLastObservedTimeThresh_ms);
      return false;
    }
    
    const bool res = markerPose.GetWithRespectTo(robot.GetPose(), markerPoseWrtRobot);
    if(!res)
    {
      PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.GetExpectedObjectMarkerPoseWrtRobot.NoClosestMarker",
                          "Failed to get closest marker pose for object %s",
                          ObjectTypeToString(_expectedObjectType));
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
      return false;
    }
    
    return true;
  }
}

}
}



