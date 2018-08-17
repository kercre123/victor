#ifndef __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
#define __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__

namespace Anki {
namespace Vector {

enum class BEIComponentID{
  AIComponent,
  Animation,
  BeatDetector,
  BehaviorContainer,
  BehaviorEvent,
  BehaviorTimerManager,
  BlockWorld,
  BackpackLightComponent,
  CubeAccel,
  CubeComms,
  CubeConnectionCoordinator,
  CubeInteractionTracker,
  CubeLight,
  CliffSensor,
  DataAccessor,
  Delegation,
  FaceWorld,
  HabitatDetector,
  Map,
  MicComponent,
  MoodManager,
  MovementComponent,
  ObjectPoseConfirmer,
  PetWorld,
  PhotographyManager,
  PowerStateManager,
  ProxSensor,
  PublicStateBroadcaster,
  SDK,
  SettingsManager,
  RobotAudioClient,
  RobotInfo,
  TextToSpeechCoordinator,
  TouchSensor,
  VariableSnapshotComponent,
  Vision,
  VisionScheduleMediator,

  Count
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
