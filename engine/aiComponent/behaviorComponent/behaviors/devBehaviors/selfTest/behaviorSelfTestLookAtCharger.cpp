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
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/sensors/rangeSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
static const std::string kAngleToTurnKey    = "AngleToTurnToSeeTarget_deg";
static const std::string kDistToDriveKey    = "DistanceToDriveToSeeTarget_mm";
static const std::string kExpectedObjectKey = "ExpectedObjectType";
static const std::string kExpectedDistKey   = "ExpectedDistance_mm";
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
                      "Missing %s key from SelfTestLookAtCharger Config", kAngleToTurnKey.c_str());
  }
  _angleToTurn = DEG_TO_RAD(angle);

  std::string objectType = "";
  res = JsonTools::GetValueOptional(config, kExpectedObjectKey, objectType);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorSelfTestLookAtCharger.Constructor.MissingConfigKey",
                      "Missing %s key from SelfTestLookAtCharger Config", kExpectedObjectKey.c_str());
  }
  _expectedObjectType = ObjectTypeFromString(objectType);

  res = JsonTools::GetValueOptional(config, kExpectedDistKey, _expectedDistanceToObject_mm);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorSelfTestLookAtCharger.Constructor.MissingConfigKey",
                      "Missing %s key from SelfTestLookAtCharger Config", kExpectedDistKey.c_str());
  }

  JsonTools::GetValueOptional(config, kDistToDriveKey, _distToDrive_mm);

  SubscribeToTags({EngineToGameTag::RobotObservedObject});
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

  // Make sure marker detection is enabled
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);

  // Record starting angle
  _startingAngle = robot.GetPose().GetRotation().GetAngleAroundZaxis();

  // Move head and lift to be able to see target marker and turn towards the target
  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(DEG_TO_RAD(0));
  MoveLiftToHeightAction* lift = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
  DriveStraightAction* drive = new DriveStraightAction(_distToDrive_mm, 60, false);
  CompoundActionParallel* liftHeadDrive = new CompoundActionParallel({lift, head, drive});

  TurnInPlaceAction* turn = new TurnInPlaceAction(_angleToTurn.ToFloat(), false);

  // After turning wait to process 10 images before trying to refine the turn
  WaitForImagesAction* wait = new WaitForImagesAction(5, VisionMode::DetectingMarkers);

  CompoundActionSequential* action = new CompoundActionSequential({liftHeadDrive, turn, wait});
  DelegateIfInControl(action, [this]() { TransitionToRefineTurnBeforeApproach(); });

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
    float visualDistanceToTarget_mm = 0.f;
    float visualAngleToTarget_rad = 0.f;
    Pose3d markerPose;
    ObjectID unused;
    const bool res = GetExpectedObjectMarkerPoseWrtRobot(markerPose, unused);
    if(res)
    {
      visualDistanceToTarget_mm = markerPose.GetTranslation().x();

      markerPose = markerPose.GetWithRespectToRoot();
      // Marker pose rotation is kind of wonky, compared to the robot's rotation they are
      // rotated 90 degrees. So when the robot is looking at a marker, you have to add
      // 90 degrees to get its rotation to match that of the robot
      // Taking the difference of these two angles tells us how much the robot needs to turn
      // to be perpendicular with the marker
      const auto angle = ((markerPose.GetRotation().GetAngleAroundZaxis() + DEG_TO_RAD(90)) -
                          robot.GetPose().GetRotation().GetAngleAroundZaxis());
      visualAngleToTarget_rad = angle.ToFloat();
    }

    bool isNewData = false;
    const auto& rangeData = robot.GetRangeSensorComponent().GetLatestRawRangeData(isNewData);

    PRINT_NAMED_WARNING("","%f %f %d", visualDistanceToTarget_mm, visualAngleToTarget_rad, isNewData);

    if(isNewData && res)
    {
      --_numRecordedReadingsLeft;
      PRINT_NAMED_WARNING("","RECORDING %u MORE", _numRecordedReadingsLeft);

      RangeSensorData data;
      data.rangeData = rangeData;
      data.visualDistanceToTarget_mm = 0;
      data.visualAngleAwayFromTarget_rad = 0;
      data.headAngle_rad = robot.GetHeadAngle();
      data.visualDistanceToTarget_mm = visualDistanceToTarget_mm;
      data.visualAngleAwayFromTarget_rad = visualAngleToTarget_rad;

      for(const auto& iter : rangeData.data)
      {
        const bool distValid = Util::IsNear(iter.processedRange_mm - SelfTestConfig::kDistanceSensorBiasAdjustment_mm,
                                            data.visualDistanceToTarget_mm,
                                            SelfTestConfig::kDistanceSensorReadingThresh_mm);

        const bool oneObject = (iter.numObjects == 1);
        bool statusValid = false;
        if(oneObject)
        {
          statusValid = (iter.readings[0].status == 0);
        }

        if(!statusValid || !distValid)
        {
          PRINT_NAMED_WARNING("BehaviorSelfTestLookAtCharger.UpdateInternal.ReadingOutsideThresh",
                              "Roi %u %u Dist: %f - %f Visual: %f Thresh: %f",
                              iter.roi,
                              (iter.numObjects > 0 ? iter.readings[0].status : 255),
                              iter.processedRange_mm,
                              SelfTestConfig::kDistanceSensorBiasAdjustment_mm,
                              data.visualDistanceToTarget_mm,
                              SelfTestConfig::kDistanceSensorReadingThresh_mm);

          SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::DISTANCE_SENSOR_OOR, SelfTestStatus::Running);
        }
      }
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

IActionRunner* BehaviorSelfTestLookAtCharger::CreateRefineTurn()
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

      Util::SafeDelete(action);
      Util::SafeDelete(turn);
      SELFTEST_SET_RESULT_WITH_RETURN_VAL(SelfTestResultCode::DISTANCE_MARKER_VISUAL_OOR, nullptr);
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
  action->AddAction(verify);

  return action;
}

void BehaviorSelfTestLookAtCharger::TransitionToRefineTurnBeforeApproach()
{
  IActionRunner* action = CreateRefineTurn();
  if(action == nullptr)
  {
    return;
  }

  // Once we are perpendicular to the marker, start recording distance sensor readings
  DelegateIfInControl(action, [this]() { TransitionToApproachMarker(); });
}

void BehaviorSelfTestLookAtCharger::TransitionToRefineTurnAfterApproach()
{
  IActionRunner* action = CreateRefineTurn();
  if(action == nullptr)
  {
    return;
  }

  // Once we are perpendicular to the marker, start recording distance sensor readings
  DelegateIfInControl(action, [this]() { TransitionToRecordSensor(); });
}

void BehaviorSelfTestLookAtCharger::TransitionToApproachMarker()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Check for the object in the world
  BlockWorldFilter filter;
  filter.AddAllowedType(ObjectType::Charger_Basic);
  std::vector<ObservableObject*> objects;
  robot.GetBlockWorld().FindLocatedMatchingObjects(filter, objects);

  if(objects.empty())
  {
    PRINT_NAMED_WARNING("BehaviorSelfTestDockWithCharger.GetExpectedObjectMarkerPoseWrtRobot.NullObject",
                        "Expected object of type %s does not exist or am not seeing it",
                        ObjectTypeToString(ObjectType::Charger_Basic));
    SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_NOT_FOUND);
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
      SELFTEST_SET_RESULT(SelfTestResultCode::CHARGER_NOT_FOUND);
    }

    // Copied from DriveToAndMountChargerAction
    DriveToObjectAction* action = new DriveToObjectAction(object->GetID(),
                                                          PreActionPose::ActionType::DOCKING,
                                                          0,
                                                          false,
                                                          0);
    action->SetPreActionPoseAngleTolerance(DEG_TO_RAD(15.f));
    action->DoPositionCheckOnPathCompletion(false);

    DelegateIfInControl(action, [this](){ TransitionToRefineTurnAfterApproach(); });
  }
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
