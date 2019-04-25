/**
 * File: BehaviorCubeDrive.cpp
 *
 * Author: Ron Barry
 * Created: 2019-01-07
 *
 * Description: A three-dimensional steering wheel - with 8 corners. Holding the cube upright,
 *              with the flashing light indicating the "forward" direction, move it around like
 *              a joystick to drive the robot. A quick, GENTLE, shake of the cube will cause the
 *              bot to jerk his lift up and down.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorCubeDrive.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/activeObjectAccel.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/block.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeAccelListeners/lowPassFilterListener.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/movementComponent.h"

#include "clad/externalInterface/messageEngineToGame.h"

#define LOG_CHANNEL "Behaviors"

namespace {
  const char* kTriggerLiftGs           = "triggerLiftGs";
  const char* kDeadZoneSize            = "deadZoneSize";
  const char* kTimeBetweenLiftActions  = "timeBetweenLiftActions";
  const char* kHighHeadAngle           = "highHeadAngle";
  const char* kLowHeadAngle            = "lowHeadAngle";
}

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::DynamicVariables::DynamicVariables()
{
  lastLiftActionTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::BehaviorCubeDrive(const Json::Value& config)
 : ICozmoBehavior(config)
{
  if (!JsonTools::GetValueOptional(config, kTriggerLiftGs, _iConfig.triggerLiftGs)) {
    _iConfig.triggerLiftGs = 0.5;
  }
  if (!JsonTools::GetValueOptional(config, kDeadZoneSize, _iConfig.deadZoneSize)) {
    _iConfig.deadZoneSize = 13.0;
  }
  if (!JsonTools::GetValueOptional(
      config, kTimeBetweenLiftActions, _iConfig.timeBetweenLiftActions)) {

    _iConfig.timeBetweenLiftActions = 0.75;
  }
  if (!JsonTools::GetValueOptional(config, kHighHeadAngle, _iConfig.highHeadAngle)) {
    _iConfig.highHeadAngle = 0.3;
  }
  if (!JsonTools::GetValueOptional(config, kLowHeadAngle, _iConfig.lowHeadAngle)) {
    _iConfig.lowHeadAngle = 0.0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::~BehaviorCubeDrive()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCubeDrive::WantsToBeActivatedBehavior() const
{
  // This method will only be called if the "modifiers" configuration caused the parent
  // WantsToBeActivated to return true. IOW, if there is a cube connection, this method
  // will be called. If there is a cube connection, we always want to run. (This won't
  // be true outside of the context of the demo. We'll likely want some toggle to 
  // activate/deactivate.)
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // This will cause the parent WantsToBeActivated to return false any time there is no cube connection.
  // Here, "Lazy" means that we don't want to manage the connection. That will be handled by the 
  // CubeConnectionCoordinator.
  modifiers.cubeConnectionRequirements = 
      BehaviorOperationModifiers::CubeConnectionRequirements::RequiredLazy;
  
  // The engine will take control of the cube lights to communicate internal state to the user, unless we
  // indicate that we want ownership. Since we're going to be using the lights to indicate where the
  // "front" of the steering "wheel" is, we don't want that distraction.
  modifiers.connectToCubeInBackground = true;

  // Don't cancel me just because I'm not delegating to someone/thing else.
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  static const char* list[] = {
    kTriggerLiftGs,
    kDeadZoneSize,
    kTimeBetweenLiftActions,
    kHighHeadAngle,
    kLowHeadAngle,
  };

  expectedKeys.insert( std::begin( list ), std::end( list ) );
}

void BehaviorCubeDrive::SetLiftState(bool up, double now=-1.0) {
  if (now >= 0.0) {
    if (now - _iConfig.timeBetweenLiftActions < _dVars.lastLiftActionTime) { 
      return;
    }
    _dVars.lastLiftActionTime = now;
  } else {
    _dVars.lastLiftActionTime = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
  }
  _dVars.liftUp = up;
  GetBEI().GetRobotInfo().GetMoveComponent().MoveLiftToHeight(
    up ? LIFT_HEIGHT_CARRY : LIFT_HEIGHT_LOWDOCK, MAX_LIFT_SPEED_RAD_PER_S, 
    MAX_LIFT_ACCEL_RAD_PER_S2, 0.1f, nullptr);

  // Set the head angle to follow the lift, somewhat. This way, if you're using the camera view to
  // drive, you have a way to look up/down, too.
  GetBEI().GetRobotInfo().GetMoveComponent().MoveHeadToAngle(
    up ? _iConfig.highHeadAngle : _iConfig.lowHeadAngle, 3.14f, 1000.0f);
}

void BehaviorCubeDrive::RestartAnimation() {
  static std::function<void(void)> restartAnimationCallback = 
      std::bind(&BehaviorCubeDrive::RestartAnimation, this);
  GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(
      _dVars.objectId,
      CubeAnimationTrigger::CubeDrive,
      restartAnimationCallback,
      true
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::OnBehaviorActivated() {
  // reset dynamic variables
  _dVars = DynamicVariables();
  SetLiftState(false);

  // Get the ObjectId of the connected cube and hold onto it so we can....
  ActiveID connectedCubeActiveId = GetBEI().GetCubeCommsComponent().GetConnectedCubeActiveId();
  Block* activeObject = GetBEI().GetBlockWorld().GetConnectedBlockByActiveID(connectedCubeActiveId);
  if(activeObject == nullptr) {
    LOG_ERROR("BehaviorCubeDrive.OnBehaviorActivated.NullObj", 
              "activeObject came back NULL: %d", connectedCubeActiveId);
    CancelSelf();
    return;
  }

  _dVars.objectId = activeObject->GetID();

  RestartAnimation();

  Vec3f filterCoeffs(1.0f, 1.0f, 1.0f);
  _dVars.filteredCubeAccel = std::make_shared<ActiveAccel>();
  _dVars.lowPassFilterListener = std::make_shared<CubeAccelListeners::LowPassFilterListener>(
      filterCoeffs,
      _dVars.filteredCubeAccel);
  GetBEI().GetCubeAccelComponent().AddListener(_dVars.objectId, _dVars.lowPassFilterListener);
}

void BehaviorCubeDrive::OnBehaviorDeactivated() {
  // Cause _dVars to stop listening to GetCubeAccelComponent:
  _dVars = DynamicVariables();

  SetLiftState(false);

  // Set left and right wheel velocities to 0.0, at maximum acceleration.
  GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(0.0f, 0.0f, 1000.0f, 1000.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::BehaviorUpdate() {
  if( IsActivated() ) {
    if (!GetBEI().GetCubeCommsComponent().IsConnectedToCube()) {
      CancelSelf();
      LOG_WARNING("BehaviorCubeDrive.BehaviorUpdate.ConnLost", "We've lost the connection to the cube.");
      return;
    }

    RestartAnimation();

    float xGs = _dVars.filteredCubeAccel->x / 9810.0f;
    float yGs = _dVars.filteredCubeAccel->y / 9810.0f;
    float zGs = _dVars.filteredCubeAccel->z / 9810.0f;
    float Gs = sqrtf(xGs*xGs + yGs*yGs + zGs*zGs);
    double now = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
    if (((Gs < 1.0f-_iConfig.triggerLiftGs) or (Gs > 1.0f+_iConfig.triggerLiftGs)) && 
       (now - _iConfig.timeBetweenLiftActions > _dVars.lastLiftActionTime)) {
      SetLiftState(!_dVars.liftUp, now);
    }

    // max speed is 200mm per second. I want the cube to be able to push the bot at full speed, but I've no
    // guarantee that the accelerometer reads 9810 at 1G, as it "should". To make up for this, as well as 
    // to compensate a bit for the dead zone, I scale the accelerometer reading (measured in Gs) by 250 to
    // get my desired wheel speed.
    float leftWheelMmps = -xGs * 250.0f;
    float rightWheelMmps = -xGs * 250.0f;

    leftWheelMmps += yGs * 250.0f;
    rightWheelMmps -= yGs * 250.0f;

    float deadZoneSize = _iConfig.deadZoneSize;
    if (abs(leftWheelMmps) < deadZoneSize) {
      leftWheelMmps = 0.0f;
    } else {
      if (leftWheelMmps < 0.0f) {
        leftWheelMmps += deadZoneSize;
      } else {
        leftWheelMmps -= deadZoneSize;
      }
    }
    if (abs(rightWheelMmps) < deadZoneSize) {
      rightWheelMmps = 0.0f;
    } else {
      if (rightWheelMmps < 0.0f) {
        rightWheelMmps += deadZoneSize;
      } else {
        rightWheelMmps -= deadZoneSize;
      }
    }

    // Set left and right wheel velocities to computed values, at maximum acceleration.
    GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(
        leftWheelMmps, rightWheelMmps, 1000.0f, 1000.0f);
  }
}

} // namespace Vector
} // namespace Anki
