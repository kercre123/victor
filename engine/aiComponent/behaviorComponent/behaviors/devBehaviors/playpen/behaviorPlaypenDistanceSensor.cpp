/**
 * File: behaviorPlaypenDistanceSensor.cpp
 *
 * Author: Al Chaussee
 * Created: 09/22/17
 *
 * Description: Turns towards a target marker and records a number of distance sensor readings
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenDistanceSensor.h"

#include "engine/actions/basicActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/sensors/rangeSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include "coretech/common/engine/jsonTools.h"

#include "whiskeyToF/tof.h"

namespace Anki {
namespace Vector {

namespace {
static const char* const kAngleToTurnKey    = "AngleToTurnToSeeTarget_deg";
static const char* const kDistToDriveKey    = "DistanceToDriveToSeeTarget_mm";
static const char* const kExpectedObjectKey = "ExpectedObjectType";
static const char* const kExpectedDistKey   = "ExpectedDistance_mm";
static const char* const kPerformCalibrationKey   = "PerformCalibration";
}

BehaviorPlaypenDistanceSensor::BehaviorPlaypenDistanceSensor(const Json::Value& config)
: IBehaviorPlaypen(config)
{
  bool res = false;
  float angle = 0;
  res = JsonTools::GetValueOptional(config, kAngleToTurnKey, angle);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kAngleToTurnKey);
  }
  _angleToTurn = DEG_TO_RAD(angle);
  
  std::string objectType = "";
  res = JsonTools::GetValueOptional(config, kExpectedObjectKey, objectType);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kExpectedObjectKey);
  }
  _expectedObjectType = ObjectTypeFromString(objectType);
  
  res = JsonTools::GetValueOptional(config, kExpectedDistKey, _expectedDistanceToObject_mm);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kExpectedDistKey);
  }

  res = JsonTools::GetValueOptional(config, kPerformCalibrationKey, _performCalibration);
  if(!res)
  {
    PRINT_NAMED_ERROR("BehaviorPlaypenDistanceSensor.Constructor.MissingConfigKey",
                      "Missing %s key from PlaypenDistanceSensor Config", kPerformCalibrationKey);
  }
  
  JsonTools::GetValueOptional(config, kDistToDriveKey, _distToDrive_mm);
  
  SubscribeToTags({EngineToGameTag::RobotObservedObject});
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaypenDistanceSensor::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAngleToTurnKey,
    kDistToDriveKey,
    kExpectedObjectKey,
    kExpectedDistKey,
    kPerformCalibrationKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

Result BehaviorPlaypenDistanceSensor::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(_expectedObjectType == ObjectType::UnknownObject)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.InternalInitInternal.NoObject",
                        "No object type specified to look for, failing");
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::CUBE_NOT_FOUND, RESULT_FAIL);
  }

  // Record starting angle
  _startingAngle = robot.GetPose().GetRotation().GetAngleAroundZaxis();

  _calibrationComplete = false;
  _calibrationRunning = false;

  // Add an extra 30 seconds to timeout time if we are calibrating since it is slow
  IncreaseTimeoutTimer(30000);

  // Move head and lift to be able to see target marker and turn towards the target
  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(DEG_TO_RAD(0));
  MoveLiftToHeightAction* lift = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
  DriveStraightAction* drive = new DriveStraightAction((_distToDrive_mm > 0 ? _distToDrive_mm : 0));
  CompoundActionParallel* liftHeadDrive = new CompoundActionParallel({lift, head, drive});

  TurnInPlaceAction* turn = new TurnInPlaceAction(_angleToTurn.ToFloat(), false);

  // After turning wait to process 10 images before trying to refine the turn
  WaitForImagesAction* wait = new WaitForImagesAction(5, VisionMode::Markers);
  
  CompoundActionSequential* action = new CompoundActionSequential({liftHeadDrive, turn, wait});
  DelegateIfInControl(action, [this]() { TransitionToRefineTurn(); });
  
  return RESULT_OK;
}

IBehaviorPlaypen::PlaypenStatus BehaviorPlaypenDistanceSensor::PlaypenUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Haven't started recording distance readings yet
  if(_numRecordedReadingsLeft == -1)
  {
    return PlaypenStatus::Running;
  }
  // We have distance readings left to record
  else if(_numRecordedReadingsLeft > 0)
  {
    bool isNewData = false;
    const auto& rangeData = robot.GetRangeSensorComponent().GetLatestRawRangeData(isNewData);
    if(isNewData)
    {
      --_numRecordedReadingsLeft;
      PRINT_NAMED_INFO("BehaviorPlaypenDistanceSensor.PlaypenUpdateInternal.Recording",
                       "Recording %u more",
                       _numRecordedReadingsLeft);
    
      RangeSensorData data;
      data.rangeData = rangeData;
      data.visualDistanceToTarget_mm = 0;
      data.visualAngleAwayFromTarget_rad = 0;
      data.headAngle_rad = robot.GetComponent<FullRobotPose>().GetHeadAngle();
      data.visualDistanceToTarget_mm = _visualDistanceToTarget_mm;
      data.visualAngleAwayFromTarget_rad = _visualAngleToTarget_rad;
    
      if(!GetLogger().Append(GetDebugLabel(), data))
      {
        PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::WRITE_TO_LOG_FAILED, PlaypenStatus::Running);
      }
    }

    // TODO NEED TO REWRITE TO CHECK ALL ROIs
    // if(robot.IsPhysical() &&
    //    !Util::IsNear(data.proxSensorData.distance_mm - PlaypenConfig::kDistanceSensorBiasAdjustment_mm, 
    //                  data.visualDistanceToTarget_mm,
    //                  PlaypenConfig::kDistanceSensorReadingThresh_mm))
    // {
    //   PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.PlaypenUpdateInternal.ReadingOutsideThresh",
    //                       "Sensor reading %u - %f not near visual reading %f with threshold %f",
    //                       data.proxSensorData.distance_mm,
    //                       PlaypenConfig::kDistanceSensorBiasAdjustment_mm,
    //                       data.visualDistanceToTarget_mm,
    //                       PlaypenConfig::kDistanceSensorReadingThresh_mm);

    //   PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::DISTANCE_SENSOR_OOR, PlaypenStatus::Running);
    // }
    
    return PlaypenStatus::Running;
  }
  // We've recorded all distance readings we need to
  else
  {
    // If we aren't acting then turn back to where we were facing when the behavior started
    if(!IsControlDelegated())
    {
      TransitionToTurnBack();
    }
    return PlaypenStatus::Running;
  }
}

void BehaviorPlaypenDistanceSensor::OnBehaviorDeactivated()
{
  ToFSensor::getInstance()->StopRanging([this](ToFSensor::CommandResult res)
                                        {
                                          if(res != ToFSensor::CommandResult::Success)
                                          {
                                            PLAYPEN_SET_RESULT(FactoryTestResultCode::STOP_TOF_FAILED);
                                          }
                                        });
  _startingAngle = 0;
  _numRecordedReadingsLeft = -1;
  _calibrationComplete = false;
  _calibrationRunning = false;
  _visualDistanceToTarget_mm = 0;
  _visualAngleToTarget_rad = 0;

}

void BehaviorPlaypenDistanceSensor::TransitionToRefineTurn()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  auto action = std::make_unique<CompoundActionSequential>();
  TurnInPlaceAction* turn = new TurnInPlaceAction(0, false);

  // Get the pose of the marker we should be seeing
  Pose3d markerPose;
  const bool res = GetExpectedObjectMarkerPoseWrtRobot(markerPose);
  if(res)
  {
    // Check that we are within expected distance to the marker
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
    
    PRINT_CH_INFO("Behaviors", "BehaviorPlaypenDistanceSensor.TransitionToRefineTurn.TurnAngle",
                     "Turning %f degrees to be perpendicular to marker",
                     angle.getDegrees());
    
    turn->SetRequestedTurnAngle(angle.ToFloat());
  }
  action->AddAction(turn);

  action->AddAction(new DriveStraightAction(-30));


  action->AddAction(new WaitForImagesAction(5, VisionMode::Markers));

  // Once we are perpendicular to the marker, start recording distance sensor readings
  DelegateIfInControl(action.release(), [this]() { TransitionToRecordSensor(); });
}

void BehaviorPlaypenDistanceSensor::TransitionToRecordSensor()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  Pose3d markerPose;
  const bool res = GetExpectedObjectMarkerPoseWrtRobot(markerPose);
  float visualDistanceToTarget_mm = _expectedDistanceToObject_mm;
  if(res)
  {
    visualDistanceToTarget_mm = markerPose.GetTranslation().x();
    _visualDistanceToTarget_mm = visualDistanceToTarget_mm;
            
    if(visualDistanceToTarget_mm <= 0)
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.TransitionToRecordSensor.ZeroDist",
                          "Visual distance to target is 0, using expected distance %f",
                          _expectedDistanceToObject_mm);
      visualDistanceToTarget_mm = _expectedDistanceToObject_mm;
    }

    markerPose = markerPose.GetWithRespectToRoot();
    // Marker pose rotation is kind of wonky, compared to the robot's rotation they are 
    // rotated 90 degrees. So when the robot is looking at a marker, you have to add
    // 90 degrees to get its rotation to match that of the robot
    // Taking the difference of these two angles tells us how much the robot needs to turn
    // to be perpendicular with the marker
    const auto angle = ((markerPose.GetRotation().GetAngleAroundZaxis() + DEG_TO_RAD(90)) - 
                        robot.GetPose().GetRotation().GetAngleAroundZaxis());
    _visualAngleToTarget_rad = angle.ToFloat();
  }

  if(_performCalibration)
  {
    CompoundActionSequential* action = new CompoundActionSequential();

    action->AddAction(new TurnInPlaceAction(DEG_TO_RAD(12), false));

    MoveHeadToAngleAction* head = new MoveHeadToAngleAction(PlaypenConfig::kDistanceSensorHeadAngle_rad);
    action->AddAction(head);

    action->AddAction(new WaitAction(3.f));


    WaitForLambdaAction* wait = new WaitForLambdaAction([this, visualDistanceToTarget_mm](Robot& robot)
      {
        const bool isCalibrating = ToFSensor::getInstance()->IsCalibrating();
        if(!_calibrationComplete)
        {
          _calibrationComplete = (!isCalibrating && _calibrationRunning);
        }

        if(!isCalibrating && !_calibrationComplete)
        {
          ToFSensor::getInstance()->PerformCalibration(visualDistanceToTarget_mm,
                                                       PlaypenConfig::kDistanceSensorTargetReflectance,
                                                       [this](ToFSensor::CommandResult res)
                                                       {
                                                         if(res != ToFSensor::CommandResult::Success)
                                                         {
                                                           PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATE_TOF_FAILED);
                                                         }
                                                       });
        }
        
        if(_calibrationComplete)
        {
          ToFSensor::getInstance()->StartRanging([this](ToFSensor::CommandResult res)
                                                 {
                                                   if(res != ToFSensor::CommandResult::Success)
                                                   {
                                                     PLAYPEN_SET_RESULT(FactoryTestResultCode::START_TOF_FAILED);
                                                   }
                                                 });
        }
        _calibrationRunning = isCalibrating;
        
        return _calibrationComplete;
      });
    action->AddAction(wait);

    action->AddAction(new WaitAction(3.f));
    
    DelegateIfInControl(action, [this]() { _numRecordedReadingsLeft = PlaypenConfig::kNumDistanceSensorReadingsToRecord; });
  }
  else
  {
    TurnInPlaceAction* turn = new TurnInPlaceAction(DEG_TO_RAD(12), false);
    MoveHeadToAngleAction* head = new MoveHeadToAngleAction(PlaypenConfig::kDistanceSensorHeadAngle_rad);
    CompoundActionSequential* action = new CompoundActionSequential({turn, head});

    DelegateIfInControl(action, [this](){
      ToFSensor::getInstance()->StartRanging([](ToFSensor::CommandResult res)
                                             {
                                               // Should already be ranging from initial setup of the sensor
                                               // so not checking res here
                                               // if(res != ToFSensor::CommandResult::Success)
                                               // {
                                               //   PLAYPEN_SET_RESULT(FactoryTestResultCode::START_TOF_FAILED);
                                               // }
                                             });

      _numRecordedReadingsLeft = PlaypenConfig::kNumDistanceSensorReadingsToRecord;
    });
  }
}

void BehaviorPlaypenDistanceSensor::TransitionToTurnBack()
{
  TurnInPlaceAction* unturn = new TurnInPlaceAction(DEG_TO_RAD(-12), false);
  DriveStraightAction* forw = new DriveStraightAction(30);
  TurnInPlaceAction* turn = new TurnInPlaceAction(_startingAngle.ToFloat(), true);
  DriveStraightAction* drive = new DriveStraightAction((_distToDrive_mm < 0 ? _distToDrive_mm : 0));
  CompoundActionSequential* action = new CompoundActionSequential();
  action->AddAction(unturn);
  action->AddAction(forw);
  action->AddAction(turn);
  action->AddAction(drive);
  DelegateIfInControl(action, [this]() { PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS); });
}

bool BehaviorPlaypenDistanceSensor::GetExpectedObjectMarkerPoseWrtRobot(Pose3d& markerPoseWrtRobot)
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
    PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.GetExpectedObjectMarkerPoseWrtRobot.NullObject",
                        "Expected object of type %s does not exist or am not seeing it",
                        ObjectTypeToString(_expectedObjectType));
    PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
    return false;
  }
  else
  {
    // Get the most recently observed object of the expected object type
    ObservableObject* object = nullptr;
    RobotTimeStamp_t t = 0;
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
      PRINT_CH_INFO("Behaviors", "BehaviorPlaypenDistanceSensor.GetExpectedObjectMarkerPoseWrtRobot.NullObject","");
      return false;
    }

    const auto& markers = object->GetMarkers();
    
    // Get the pose of the marker that was most recently observed
    Pose3d markerPose;
    RobotTimeStamp_t lastObservedTime = 0;
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

    if(!Util::IsNear((float)lastObservedTime, (float)robot.GetLastImageTimeStamp(), 500.f))
    {
      PRINT_CH_INFO("Behaviors",
                    "BehaviorPlaypenDistanceSensor.GetExpectedObjectMarkerPoseWrtRobot.MarkerTooOld",
                    "%u %u",
                    (u32)lastObservedTime,
                    (u32)robot.GetLastImageTimeStamp());
      return false;
    }
    
    const bool res = markerPose.GetWithRespectTo(robot.GetPose(), markerPoseWrtRobot);
    if(!res)
    {
      PRINT_NAMED_WARNING("BehaviorPlaypenDistanceSensor.GetExpectedObjectMarkerPoseWrtRobot.NoClosestMarker",
                          "Failed to get closest marker pose for object %s",
                          ObjectTypeToString(_expectedObjectType));
      PLAYPEN_SET_RESULT_WITH_RETURN_VAL(FactoryTestResultCode::DISTANCE_MARKER_NOT_FOUND, false);
      return false;
    }
    
    return true;
  }
}

}
}


