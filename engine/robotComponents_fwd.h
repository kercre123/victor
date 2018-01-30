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

#include "engine/dependencyManagedComponent.h"

namespace Anki {
namespace Cozmo {

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

using RobotComp =  DependencyManagedComponentWrapper<RobotComponentID>;
using RobotCompMap = std::map<RobotComponentID, RobotComp>;
using RobotCompIDSet = std::set<RobotComponentID>;
} // namespace Cozmo
} // namespace Anki

#endif // __Engine_RobotComponentsFWD_H__
