/**
* File: robotComponents_fwd.h
*
* Author: Kevin M. Karol
* Created: 1/5/18
*
* Description: Enumeration of components within robot.h's entity
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_RobotComponentsFWD_H__
#define __Engine_RobotComponentsFWD_H__

#include "util/entityComponent/iDependencyManagedComponent.h"

namespace Anki {
namespace Cozmo {

// When adding to this enum be sure to also declare a template specialization
// in the _impl.cpp file mapping the enum to the class type it is associated with
enum class RobotComponentID{
  CozmoContext,
  BlockWorld,
  FaceWorld,
  PetWorld,
  PublicStateBroadcaster,
  EngineAudioClient,
  PathPlanning,
  DrivingAnimationHandler,
  ActionList,
  Movement,
  Vision,
  VisionScheduleMediator,
  Map,
  NVStorage,
  AIComponent,
  ObjectPoseConfirmer,
  CubeLights,
  BodyLights,
  CubeAccel,
  CubeComms,
  GyroDriftDetector,
  Docking,
  Carrying,
  CliffSensor,
  ProxSensor,
  TouchSensor,
  Animation,
  StateHistory,
  MoodManager,
  Inventory,
  ProgressionUnlock,
  BlockTapFilter,
  RobotToEngineImplMessaging,
  RobotIdleTimeout,
  MicDirectionHistory,
  Count
};

using RobotComp =  IDependencyManagedComponent<RobotComponentID>;
using RobotCompMap = DependencyManagedEntity<RobotComponentID>;
using RobotCompIDSet = std::set<RobotComponentID>;

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_RobotComponentsFWD_H__
