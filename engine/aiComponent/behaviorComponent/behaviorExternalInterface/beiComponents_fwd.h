#ifndef __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
#define __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__

namespace Anki {
namespace Cozmo {

enum class BEIComponentID{
  AIComponent,
  Animation,
  BeatDetector,
  BehaviorContainer,
  BehaviorEvent,
  BehaviorTimerManager,
  BlockWorld,
  BodyLightComponent,
  CubeAccel,
  CubeLight,
  DataAccessor,
  Delegation,
  FaceWorld,
  Map,
  MicComponent,
  MoodManager,
  MovementComponent,
  ObjectPoseConfirmer,
  PetWorld,
  ProgressionUnlock,
  ProxSensor,
  PublicStateBroadcaster,
  RobotAudioClient,
  RobotInfo,
  TouchSensor,
  Vision,
  VisionScheduleMediator,

  Count
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
