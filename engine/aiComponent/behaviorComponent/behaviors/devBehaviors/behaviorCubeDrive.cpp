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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/block.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeAccelListeners/lowPassFilterListener.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/movementComponent.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::DynamicVariables::DynamicVariables()
{
  last_lift_action_time = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCubeDrive::BehaviorCubeDrive(const Json::Value& config)
 : ICozmoBehavior(config)
{
  if (not JsonTools::GetValueOptional(config, kTriggerLiftGs, _iConfig.trigger_lift_gs)) {
    _iConfig.trigger_lift_gs = 0.5;
  }
  if (not JsonTools::GetValueOptional(config, kDeadZoneSize, _iConfig.dead_zone_size)) {
    _iConfig.dead_zone_size = 13.0;
  }
  if (not JsonTools::GetValueOptional(
      config, kTimeBetweenLiftActions, _iConfig.time_between_lift_actions)) {

    _iConfig.time_between_lift_actions = 0.75;
  }
  if (not JsonTools::GetValueOptional(config, kHighHeadAngle, _iConfig.high_head_angle)) {
    _iConfig.high_head_angle = 0.3;
  }
  if (not JsonTools::GetValueOptional(config, kLowHeadAngle, _iConfig.low_head_angle)) {
    _iConfig.low_head_angle = 0.0;
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
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::RequiredLazy;
  
  // The engine will take control of the cube lights to communicate internal state to the user, unless we
  // indicate that we want ownership. Since we're going to be using the lights to indicate where the "front"
  // of the steering "wheel" is, we don't want that distraction.
  modifiers.connectToCubeInBackground = true;

  // Don't cancel me just because I'm not delegating to someone/thing else.
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
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

void BehaviorCubeDrive::SetLiftState(bool up) {
  _dVars.lift_up = up;
  GetBEI().GetRobotInfo().GetMoveComponent().MoveLiftToHeight(
    up ? LIFT_HEIGHT_CARRY : LIFT_HEIGHT_LOWDOCK, MAX_LIFT_SPEED_RAD_PER_S, MAX_LIFT_ACCEL_RAD_PER_S2, 0.1, nullptr);

  // Set the head angle to follow the lift, somewhat. This way, if you're using the camera view to
  // drive, you have a way to look up/down, too.
  GetBEI().GetRobotInfo().GetMoveComponent().MoveHeadToAngle(
    up ? _iConfig.high_head_angle : _iConfig.low_head_angle, 3.14, 1000.0);
}

void BehaviorCubeDrive::RestartAnimation() {
  static std::function<void(void)> restart_animation_callback = std::bind(&BehaviorCubeDrive::RestartAnimation, this);
  GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(
      _dVars.object_id,
      CubeAnimationTrigger::CubeDrive,
      restart_animation_callback,
      true
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::OnBehaviorActivated() {
  // reset dynamic variables
  _dVars = DynamicVariables();
  SetLiftState(false);

  // Get the ObjectId of the connected cube and hold onto it so we can....
  ActiveID connected_cube_active_id = GetBEI().GetCubeCommsComponent().GetConnectedCubeActiveId();
  Block* active_object = GetBEI().GetBlockWorld().GetConnectedBlockByActiveID(connected_cube_active_id);

  if(active_object == nullptr) {
    LOG_ERROR("cube_drive", "active_object came back NULL: %d", connected_cube_active_id);
    CancelSelf();
    return;
  }

  _dVars.object_id = active_object->GetID();

  RestartAnimation();

  Vec3f filter_coeffs(1.0, 1.0, 1.0);
  _dVars.filtered_cube_accel = std::make_shared<ActiveAccel>();
  _dVars.low_pass_filter_listener = std::make_shared<CubeAccelListeners::LowPassFilterListener>(
      filter_coeffs,
      _dVars.filtered_cube_accel);
  GetBEI().GetCubeAccelComponent().AddListener(_dVars.object_id, _dVars.low_pass_filter_listener);
}

void BehaviorCubeDrive::OnBehaviorDeactivated() {
  _dVars = DynamicVariables();
  SetLiftState(false);

  // Set left and right wheel velocities to 0.0, at maximum acceleration.
  GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(0.0, 0.0, 1000.0, 1000.0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::BehaviorUpdate() {
  if( IsActivated() ) {
    if(not GetBEI().GetCubeCommsComponent().IsConnectedToCube()) {
      CancelSelf();
      LOG_WARNING("cube_drive", "We've lost the connection to the cube");
      return;
    }

    RestartAnimation();

    float xGs = _dVars.filtered_cube_accel->x / 9810.0;
    float yGs = _dVars.filtered_cube_accel->y / 9810.0;
    float zGs = _dVars.filtered_cube_accel->z / 9810.0;
    float Gs = sqrt(xGs*xGs + yGs*yGs + zGs*zGs);
    double now = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
    if((Gs < 1.0-_iConfig.trigger_lift_gs or Gs > 1.0+_iConfig.trigger_lift_gs) && 
       (now - _iConfig.time_between_lift_actions > _dVars.last_lift_action_time)) {
      _dVars.last_lift_action_time = now;
      SetLiftState(not _dVars.lift_up);
    }

    // max speed is 200mm per second. I want the cube to be able to push the bot at full speed, but I've no
    // guarantee that the accelerometer reads 9810 at 1G, as it "should". To make up for this, as well as to
    // compensate a bit for the dead zone, I scale the accelerometer reading (measured in Gs) by 250 to get my
    // desired wheel speed.
    float left_wheel_mmps = -xGs * 250.0;
    float right_wheel_mmps = -xGs * 250.0;

    left_wheel_mmps += yGs * 250.0;
    right_wheel_mmps -= yGs * 250.0;

    float dead_zone_size = _iConfig.dead_zone_size;
    if(abs(left_wheel_mmps) < dead_zone_size) {
      left_wheel_mmps = 0.0;
    } else {
      left_wheel_mmps -= dead_zone_size;
    }
    if(abs(right_wheel_mmps) < dead_zone_size) {
      right_wheel_mmps = 0.0;
    } else {
      right_wheel_mmps -= dead_zone_size;
    }

    // Set left and right wheel velocities to computed values, at maximum acceleration.
    GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(
        left_wheel_mmps, right_wheel_mmps, 1000.0, 1000.0);
  }
}

} // namespace Vector
} // namespace Anki
