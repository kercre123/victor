/**
* File: beiRobotInfo.h
*
* Author: Kevin M. Karol
* Created: 11/17/17
*
* Description: Wrapper that hides robot from the BEI and only exposes information
* that has been deemed appropriate for the BEI to access
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_BEIRobotInfo_H__
#define __Cozmo_Basestation_BehaviorSystem_BEIRobotInfo_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/pose.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "clad/types/batteryTypes.h"
#include "clad/types/offTreadsStates.h"

// forward declaration
class Quad2f;

namespace Anki {

class PoseOriginList;
namespace Util {
class RandomGenerator;

namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

// forward declaration
class BatteryComponent;
class CarryingComponent;
class CliffSensorComponent;
class CozmoContext;
class DockingComponent;
class DrivingAnimationHandler;
class IExternalInterface;
class MoodManager;
class MovementComponent;
class ObjectPoseConfirmer;
class PathComponent;
class ProgressionUnlockComponent;
class PublicStateBroadcaster;
class Robot;
class VisionComponent;

struct AccelData;
struct GyroData;

  
class BEIRobotInfo : public IDependencyManagedComponent<BCComponentID> {
public:
  BEIRobotInfo(Robot& robot)
  : IDependencyManagedComponent(this, BCComponentID::RobotInfo)
  , _robot(robot){};
  virtual ~BEIRobotInfo();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComponents) override {};
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////


  BatteryLevel GetBatteryLevel() const;
  Quad2f GetBoundingQuadXY(const Pose3d& atPose) const;
  CarryingComponent& GetCarryingComponent() const;
  const CliffSensorComponent& GetCliffSensorComponent() const;
  const CozmoContext* GetContext() const;
  u32 GetDisplayWidthInPixels() const;
  u32 GetDisplayHeightInPixels() const;
  DockingComponent& GetDockingComponent() const;
  DrivingAnimationHandler& GetDrivingAnimationHandler() const;
  const AccelData& GetHeadAccelData() const;
  float GetHeadAccelMagnitudeFiltered() const;
  const f32 GetHeadAngle() const;
  const GyroData& GetHeadGyroData() const;
  const RobotID_t GetID() const;
  TimeStamp_t GetLastChargingStateChangeTimestamp() const;
  TimeStamp_t GetLastImageTimeStamp() const;  
  TimeStamp_t GetLastMsgTimestamp() const;
  f32 GetLiftAngle() const;
  f32 GetLiftHeight() const;
  MovementComponent& GetMoveComponent() const;
  ObjectPoseConfirmer& GetObjectPoseConfirmer() const;
  OffTreadsState GetOffTreadsState() const;
  PathComponent& GetPathComponent() const;
  Radians GetPitchAngle() const;
  const Pose3d& GetPose() const;
  const PoseOriginList&  GetPoseOriginList() const;
  Util::RandomGenerator& GetRNG();
  const Pose3d& GetWorldOrigin()  const;
  PoseOriginID_t GetWorldOriginID() const;
  u32 GetHeadSerialNumber() const;
  Util::Data::DataPlatform* GetDataPlatform() const;
  bool IsPowerButtonPressed() const;

  bool HasExternalInterface() const;
  IExternalInterface* GetExternalInterface();

  Result ComputeHeadAngleToSeePose(const Pose3d& pose, Radians& headAngle, f32 yTolFrac) const;

  bool IsCharging() const;
  bool IsHeadCalibrated() const;
  bool IsLiftCalibrated() const;
  bool IsOnChargerContacts() const;
  bool IsOnChargerPlatform() const;
  bool IsPhysical() const;
  bool IsPickedUp() const;
  bool IsPoseInWorldOrigin(const Pose3d& pose) const;

  void EnableStopOnCliff(const bool enable);
  void StartDoom();
  
private:
  // let the test classes access robot directly
  friend class BehaviorFactoryCentroidExtractor;
  friend class BehaviorFactoryTest;
  friend class BehaviorDockingTestSimple;
  friend class BehaviorLiftLoadTest;

  friend class IBehaviorPlaypen;
  friend class BehaviorPlaypenInitChecks;
  friend class BehaviorPlaypenDriftCheck;
  friend class BehaviorPlaypenPickupCube;
  friend class BehaviorPlaypenMotorCalibration;
  friend class BehaviorPlaypenDistanceSensor;
  friend class BehaviorPlaypenSoundCheck;
  friend class BehaviorPlaypenEndChecks;
  friend class BehaviorPlaypenDriveForwards;
  friend class BehaviorPlaypenTest;
  friend class BehaviorPlaypenWaitToStart;
  friend class BehaviorPlaypenCameraCalibration;
  friend class BehaviorPlaypenReadToolCode;

  Robot& _robot;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEIRobotInfo_H__
