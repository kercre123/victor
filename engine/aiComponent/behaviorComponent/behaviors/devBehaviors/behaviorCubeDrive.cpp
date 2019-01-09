/**
 * File: BehaviorCubeDrive.cpp
 *
 * Author: Ron Barry
 * Created: 2019-01-07
 *
 * Description: A three-dimensional steering wheel - with 8 corners.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/activeObjectAccel.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorCubeDrive.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeAccelListeners/lowPassFilterListener.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/movementComponent.h"

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
  // TODO: read config into _iConfig
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
  /*
  const char* list[] = {
    // TODO: insert any possible root-level json keys that this class is expecting.
    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  */
}

void BehaviorCubeDrive::SetLiftState(bool up) {
  _dVars.lift_up = up;
  GetBEI().GetRobotInfo().GetMoveComponent().MoveLiftToHeight(
    up ? LIFT_HEIGHT_CARRY : LIFT_HEIGHT_LOWDOCK, MAX_LIFT_SPEED_RAD_PER_S, MAX_LIFT_ACCEL_RAD_PER_S2, 0.1, nullptr);
}

void BehaviorCubeDrive::RestartAnimation() {
  static std::function<void(void)> restart_animation_callback = std::bind(&BehaviorCubeDrive::RestartAnimation, this);
  bool retval = GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(
      _dVars.object_id,
      CubeAnimationTrigger::CubeDrive,
      restart_animation_callback
  );  
  LOG_WARNING("cube_drive", "RestartAnimation() PlayLightAnimByTrigerr() returned: %d", retval);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::OnBehaviorActivated() {
  LOG_WARNING("cube_drive", "OnBehaviorActivated()");

  // reset dynamic variables
  _dVars = DynamicVariables();
  SetLiftState(false);
  
  // Get the ObjectId of the connected cube and hold onto it so we can....
  ActiveID connected_cube_active_id = GetBEI().GetCubeCommsComponent().GetConnectedCubeActiveId();
  ActiveObject* active_object = GetBEI().GetBlockWorld().GetConnectedActiveObjectByActiveID(connected_cube_active_id);
  
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
  LOG_WARNING("cube_drive", "OnBehaviorDeactivated()");
  _dVars = DynamicVariables();
  SetLiftState(false);

  GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(0.0, 0.0, 1000.0, 1000.0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCubeDrive::BehaviorUpdate() {
  if( IsActivated() ) {
    if(not GetBEI().GetCubeCommsComponent().IsConnectedToCube()) {
      CancelSelf();
      LOG_ERROR("cube_drive", "We've lost the connection to the cube");
      return;
    }
    float xGs = _dVars.filtered_cube_accel->x / 9810.0;
    float yGs = _dVars.filtered_cube_accel->y / 9810.0;
    float left_wheel_mmps = -xGs * 250.0;
    float right_wheel_mmps = -xGs * 250.0;

    left_wheel_mmps += yGs * 250.0;
    right_wheel_mmps -= yGs * 250.0;

    if(abs(left_wheel_mmps)<8.0) {
      left_wheel_mmps = 0.0;
    }
    if(abs(right_wheel_mmps)<8.0) {
      right_wheel_mmps = 0.0;
    }
     
    GetBEI().GetRobotInfo().GetMoveComponent().DriveWheels(left_wheel_mmps, right_wheel_mmps, 1000.0, 1000.0);

    double now = BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble();
    if(now - 0.25 > _dVars.last_lift_action_time) {
      if(_dVars.filtered_cube_accel->z > 9810.0 * 2.0) {
        _dVars.last_lift_action_time = now;
        SetLiftState(true);
      } else if(_dVars.filtered_cube_accel->z < -9810.0) {
        _dVars.last_lift_action_time = now;
        SetLiftState(false);
      }
    }
  }
}

} // namespace Vector
} // namespace Anki
