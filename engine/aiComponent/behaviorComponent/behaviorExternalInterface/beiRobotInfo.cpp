/**
* File: beiRobotInfo.cpp
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

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "engine/robot.h"
#include "engine/components/batteryComponent.h"
#include "engine/components/carryingComponent.h"

#include "osState/osState.h"

namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIRobotInfo::~BEIRobotInfo()
{
  
}
  
ActionList& BEIRobotInfo::GetActionList()
{
  return _robot.GetComponent<ActionList>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BatteryLevel BEIRobotInfo::GetBatteryLevel() const
{
  return _robot.GetBatteryComponent().GetBatteryLevel();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Quad2f BEIRobotInfo::GetBoundingQuadXY(const Pose3d& atPose) const
{
  return _robot.GetBoundingQuadXY(atPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CarryingComponent& BEIRobotInfo::GetCarryingComponent() const
{
  return _robot.GetCarryingComponent();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsCarryingObject() const
{
  return GetCarryingComponent().IsCarryingObject();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const CliffSensorComponent& BEIRobotInfo::GetCliffSensorComponent() const
{
  return _robot.GetCliffSensorComponent();
}

const ProxSensorComponent& BEIRobotInfo::GetProxSensorComponent() const
{
  return _robot.GetProxSensorComponent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const CozmoContext* BEIRobotInfo::GetContext() const
{
  return _robot.GetContext();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 BEIRobotInfo::GetDisplayWidthInPixels() const
{
  return _robot.GetDisplayWidthInPixels();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 BEIRobotInfo::GetDisplayHeightInPixels() const
{
  return _robot.GetDisplayHeightInPixels();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DockingComponent& BEIRobotInfo::GetDockingComponent() const
{
  return _robot.GetDockingComponent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DrivingAnimationHandler& BEIRobotInfo::GetDrivingAnimationHandler() const
{
  return _robot.GetDrivingAnimationHandler();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AccelData& BEIRobotInfo::GetHeadAccelData() const
{
  return _robot.GetHeadAccelData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BEIRobotInfo::GetHeadAccelMagnitudeFiltered() const
{
  return _robot.GetHeadAccelMagnitudeFiltered();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const f32 BEIRobotInfo::GetHeadAngle() const
{
  return _robot.GetComponent<FullRobotPose>().GetHeadAngle();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GyroData& BEIRobotInfo::GetHeadGyroData() const
{
  return _robot.GetHeadGyroData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const RobotID_t BEIRobotInfo::GetID() const
{
  return _robot.GetID();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotTimeStamp_t BEIRobotInfo::GetLastImageTimeStamp() const
{
  return _robot.GetLastImageTimeStamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotTimeStamp_t BEIRobotInfo::GetLastMsgTimestamp() const
{
  return _robot.GetLastMsgTimestamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
f32 BEIRobotInfo::GetLiftAngle() const
{
  return _robot.GetComponent<FullRobotPose>().GetLiftAngle();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
f32 BEIRobotInfo::GetLiftHeight() const
{
  return _robot.GetLiftHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MovementComponent& BEIRobotInfo::GetMoveComponent() const
{
  return _robot.GetMoveComponent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectPoseConfirmer& BEIRobotInfo::GetObjectPoseConfirmer() const
{
  return _robot.GetObjectPoseConfirmer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffTreadsState BEIRobotInfo::GetOffTreadsState() const
{
  return _robot.GetOffTreadsState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EngineTimeStamp_t BEIRobotInfo::GetOffTreadsStateLastChangedTime_ms() const
{
  return _robot.GetOffTreadsStateLastChangedTime_ms();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PathComponent& BEIRobotInfo::GetPathComponent() const
{
  return _robot.GetPathComponent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Radians BEIRobotInfo::GetPitchAngle() const
{
  return _robot.GetPitchAngle();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Pose3d& BEIRobotInfo::GetPose() const
{
  return _robot.GetPose();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const PoseOriginList& BEIRobotInfo::GetPoseOriginList() const
{
  return _robot.GetPoseOriginList();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotEventHandler& BEIRobotInfo::GetRobotEventHandler() const
{
  return _robot.GetRobotEventHandler();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& BEIRobotInfo::GetRNG()
{
  return _robot.GetRNG();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SDKComponent& BEIRobotInfo::GetSDKComponent() const
{
  return _robot.GetSDKComponent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Pose3d& BEIRobotInfo::GetWorldOrigin()  const
{
  return _robot.GetWorldOrigin();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PoseOriginID_t BEIRobotInfo::GetWorldOriginID() const
{
  return _robot.GetWorldOriginID();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsPowerButtonPressed() const
{
  return _robot.IsPowerButtonPressed();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BEIRobotInfo::GetTimeSincePowerButtonPressed_ms() const
{
  return _robot.GetTimeSincePowerButtonPressed_ms();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::HasExternalInterface() const
{
  return _robot.HasExternalInterface();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IExternalInterface* BEIRobotInfo::GetExternalInterface()
{
  return _robot.GetExternalInterface();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::HasGatewayInterface() const
{
  return _robot.HasGatewayInterface();
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t BEIRobotInfo::GetCpuTemperature_degC() const
{
  return OSState::getInstance()->GetTemperature_C();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IGatewayInterface* BEIRobotInfo::GetGatewayInterface()
{
  return _robot.GetGatewayInterface();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BEIRobotInfo::ComputeHeadAngleToSeePose(const Pose3d& pose, Radians& headAngle, f32 yTolFrac) const
{
  return _robot.ComputeHeadAngleToSeePose(pose, headAngle, yTolFrac);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsCharging() const
{
  return _robot.GetBatteryComponent().IsCharging();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsHeadCalibrated() const
{
  return _robot.IsHeadCalibrated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsLiftCalibrated() const
{
  return _robot.IsLiftCalibrated();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsHeadMotorOutOfBounds() const
{
  return _robot.IsHeadMotorOutOfBounds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsLiftMotorOutOfBounds() const
{
  return _robot.IsLiftMotorOutOfBounds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsOnChargerContacts() const
{
  return _robot.GetBatteryComponent().IsOnChargerContacts();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsOnChargerPlatform() const
{
  return _robot.GetBatteryComponent().IsOnChargerPlatform();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsPhysical() const
{
  return _robot.IsPhysical();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsPickedUp() const
{
  return _robot.IsPickedUp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 BEIRobotInfo::GetTimeSinceLastPoke_ms() const
{
  return static_cast<u32>(_robot.GetTimeSinceLastPoke_ms());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsBeingHeld() const
{
  return _robot.IsBeingHeld();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIRobotInfo::IsPoseInWorldOrigin(const Pose3d& pose) const
{
  return _robot.IsPoseInWorldOrigin(pose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BEIRobotInfo::EnableStopOnCliff(const bool enable)
{
  _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(enable)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 BEIRobotInfo::GetHeadSerialNumber() const
{
  return _robot.GetHeadSerialNumber();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::Data::DataPlatform* BEIRobotInfo::GetDataPlatform() const
{
  return _robot.GetContextDataPlatform();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NVStorageComponent& BEIRobotInfo::GetNVStorageComponent() const
{
  return _robot.GetNVStorageComponent();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BatteryComponent& BEIRobotInfo::GetBatteryComponent() const
{
  return _robot.GetBatteryComponent();
}

  
} // namespace Vector
} // namespace Anki
