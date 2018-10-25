/**
 * File: behaviorPlaypenPickupCube.cpp
 *
 * Author: Al Chaussee
 * Created: 08/08/17
 *
 * Description: Checks that Cozmo can pickup and place LightCube1 (paperclip) with minimal changes in body rotation
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenPickupCube.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {

namespace {
  static const Pose3d kExpectedCubePose = Pose3d(0, Z_AXIS_3D(), 
                                                 {PlaypenConfig::kExpectedCubePoseX_mm, 
                                                  PlaypenConfig::kExpectedCubePoseY_mm, 
                                                  22}, 
                                                 "");

  // Rotation ambiguities for observed blocks.
  // We only care that the block is upright.
  static const RotationAmbiguities kBlockRotationAmbiguities(true, {
    RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
    RotationMatrix3d({0,1,0,  1,0,0,  0,0,1})
  });
}

BehaviorPlaypenPickupCube::BehaviorPlaypenPickupCube(const Json::Value& config)
: IBehaviorPlaypen(config)
{
  ICozmoBehavior::SubscribeToTags({RobotInterface::RobotToEngineTag::pickAndPlaceResult});
}

PoseData ConvertToPoseData(const Pose3d& p)
{
  PoseData poseData;
  Radians angleX, angleY, angleZ;
  p.GetRotationMatrix().GetEulerAngles(angleX, angleY, angleZ);
  poseData.angleX_rad = angleX.ToFloat();
  poseData.angleY_rad = angleY.ToFloat();
  poseData.angleZ_rad = angleZ.ToFloat();
  poseData.transX_mm = p.GetTranslation().x();
  poseData.transY_mm = p.GetTranslation().y();
  poseData.transZ_mm = p.GetTranslation().z();
  return poseData;
}

Result BehaviorPlaypenPickupCube::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  RecordTouchSensorData(robot, GetDebugLabel());
  
  // Make sure marker detection is enabled (probably super overkill...)
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);

  MoveHeadToAngleAction* head = new MoveHeadToAngleAction(DEG_TO_RAD(0));
  // TurnInPlaceAction* turn = new TurnInPlaceAction(robot, DEG_TO_RAD(-90), false);
  // CompoundActionParallel* action = new CompoundActionParallel(robot, {head, turn});
  DelegateIfInControl(head, [this](){ TransitionToWaitForCube(); });

  return RESULT_OK;
}

void BehaviorPlaypenPickupCube::TransitionToWaitForCube()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Clear all objects from blockworld since the marker for the distance sensor check creates an
  // object that is in the way of the pickup object's predock pose
  BlockWorldFilter filter;
  filter.SetFilterFcn(nullptr);
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  robot.GetBlockWorld().DeleteLocatedObjects(filter);

  WaitForImagesAction* action = new WaitForImagesAction(2, VisionMode::DetectingMarkers);
  DelegateIfInControl(action, [this](){ TransitionToPickupCube(); });
}

void BehaviorPlaypenPickupCube::TransitionToPickupCube()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  _expectedCubePose = kExpectedCubePose;
  _expectedCubePose.SetParent(robot.GetWorldOrigin());
  
  BlockWorldFilter filter;
  filter.SetAllowedTypes({ObjectType::Block_LIGHTCUBE1});
  
  ObservableObject* object = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  
  if(object == nullptr)
  {
    PLAYPEN_SET_RESULT(FactoryTestResultCode::CUBE_NOT_FOUND);
    
    // Should we be ignoring playpen failures we will need a valid object to do stuff with so make
    // a ghost object
    object = new Block_Cube1x1(ObjectType::Block_LIGHTCUBE_GHOST);
    object->InitPose(Pose3d(), PoseState::Known);
  }
  
  const Pose3d& cubePose = object->GetPose();
  PoseData poseData = ConvertToPoseData(cubePose);
  
  // Store to robot
  WriteToStorage(robot, NVStorage::NVEntryTag::NVEntry_ObservedCubePose, (u8*)&poseData, sizeof(poseData),
                 FactoryTestResultCode::CUBE_POSE_WRITE_FAILED);
  
  // Write pose data to log on device
  if(!GetLogger().AppendObservedCubePose(poseData))
  {
    PLAYPEN_SET_RESULT(FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  }
  
  // Verify that block is approximately where expected
  Vec3f Tdiff;
  Radians angleDiff;
  if (!object->GetPose().IsSameAs_WithAmbiguity(_expectedCubePose,
                                                kBlockRotationAmbiguities,
                                                PlaypenConfig::kExpectedCubePoseDistThresh_mm,
                                                PlaypenConfig::kExpectedCubePoseAngleThresh_rad,
                                                Tdiff, angleDiff))
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenPickupCube.InternalInitInternal.CubeNotWhereExpected",
                        "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f; tdiff: %f %f %f; angleDiff (deg): %f",
                        object->GetPose().GetTranslation().x(),
                        object->GetPose().GetTranslation().y(),
                        object->GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                        kExpectedCubePose.GetTranslation().x(),
                        kExpectedCubePose.GetTranslation().y(),
                        kExpectedCubePose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                        Tdiff.x(), Tdiff.y(), Tdiff.z(),
                        angleDiff.getDegrees());
    
    PLAYPEN_SET_RESULT(FactoryTestResultCode::CUBE_NOT_WHERE_EXPECTED);
  }
  
  const f32 kObservedCubePoseHeightDiff_mm = object->GetPose().GetTranslation().z() - (0.5f*object->GetSize().z());
  if(std::fabsf(kObservedCubePoseHeightDiff_mm) >PlaypenConfig:: kExpectedCubePoseHeightThresh_mm)
  {
    const f32 kObservedCubePoseHeight_mm = object->GetPose().GetTranslation().z();
    
    PRINT_NAMED_WARNING("BehaviorPlaypenPickupCube.InternalInitInternal.CubeNotWhereExpectedZ", "%f %f",
                        kObservedCubePoseHeight_mm,
                        (0.5f*object->GetSize().z()));
    
    if(kObservedCubePoseHeight_mm > kExpectedCubePose.GetTranslation().z())
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::CUBE_HEIGHT_TOO_HIGH);
    }
    else
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::CUBE_HEIGHT_TOO_LOW);
    }
  }
  
  PickupObjectAction* action = new PickupObjectAction(object->GetID());
  action->SetMotionProfile(DEFAULT_PATH_MOTION_PROFILE);
  action->SetDoNearPredockPoseCheck(true);
  action->SetShouldCheckForObjectOnTopOf(false);
  action->SetDoLiftLoadCheck(true);
  
  DelegateIfInControl(action, [this, objectID = object->GetID()](ActionResult result) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;
    
    
    if(!(result == ActionResult::SUCCESS &&
         robot.GetCarryingComponent().GetCarryingObject() == objectID))
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::PICKUP_FAILED)
    }
    
    TransitionToPlaceCube();
  });
  
  if(object->GetType() == ObjectType::Block_LIGHTCUBE_GHOST)
  {
    Util::SafeDelete(object);
    object = nullptr;
  }
}

void BehaviorPlaypenPickupCube::TransitionToPlaceCube()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  Radians robotAngleAfterBackup = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
  f32 angleChange_rad = std::fabsf((robotAngleAfterBackup - _robotAngleAtPickup).ToFloat());
  
  PRINT_CH_INFO("Behaviors", "BehaviorPlaypenPickupCube.TransitionToPlaceCube.AngleChangeDuringBackup",
                   "%f deg", RAD_TO_DEG(angleChange_rad));
  
  if (angleChange_rad > PlaypenConfig::kMaxRobotAngleChangeDuringBackup_rad)
  {
    PLAYPEN_SET_RESULT(FactoryTestResultCode::BACKUP_NOT_STRAIGHT);
  }
  
  PlaceObjectOnGroundAction* action = new PlaceObjectOnGroundAction();
  DelegateIfInControl(action, [this](ActionResult result) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    if(!(result == ActionResult::SUCCESS &&
         !robot.GetCarryingComponent().IsCarryingObject()))
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::PLACEMENT_FAILED);
    }
    
    TransitionToBackup();
  });
}

void BehaviorPlaypenPickupCube::TransitionToBackup()
{
  PlayAnimationAction* action = new PlayAnimationAction("anim_triple_backup");
  DelegateIfInControl(action, [this]() {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    // Check that robot orientation didn't change much since it lifted up the block
    Radians robotAngleAfterBackAndForth = robot.GetPose().GetRotation().GetAngleAroundZaxis();
    f32 angleChange_rad = std::fabsf((robotAngleAfterBackAndForth - _robotAngleAtPickup).ToFloat());
    
    PRINT_CH_INFO("Behaviors", "BehaviorPlaypenPickupCube.TransitionToBackup.AngleChangeDuringBackAndForth",
                     "%f deg", RAD_TO_DEG(angleChange_rad));
    
    if (angleChange_rad > PlaypenConfig::kMaxRobotAngleChangeDuringBackup_rad)
    {
      PLAYPEN_SET_RESULT(FactoryTestResultCode::BACK_AND_FORTH_NOT_STRAIGHT);
    }
    
    PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS);
  });
}

void BehaviorPlaypenPickupCube::OnBehaviorDeactivated()
{
  _robotAngleAtPickup = 0;
}

void BehaviorPlaypenPickupCube::HandleWhileActivatedInternal(const RobotToEngineEvent& event)
{
  const auto& tag = event.GetData().GetTag();
  if(tag == RobotInterface::RobotToEngineTag::pickAndPlaceResult)
  {
    const auto& payload = event.GetData().Get_pickAndPlaceResult();
    if(payload.didSucceed && payload.blockStatus == BlockStatus::BLOCK_PICKED_UP)
    {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = GetBEI().GetRobotInfo()._robot;
      
      _robotAngleAtPickup = robot.GetPose().GetRotation().GetAngleAroundZaxis();
    }
    // Pickup action will handle pickup failure
  }
}

}
}


