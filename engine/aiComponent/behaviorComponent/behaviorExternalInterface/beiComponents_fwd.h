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
  BackpackLightComponent,
  CubeAccel,
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
  ProgressionUnlock,
  ProxSensor,
  PublicStateBroadcaster,
  SDK,
  SettingsManager,
  RobotAudioClient,
  RobotInfo,
  TextToSpeechCoordinator,
  TouchSensor,
  Vision,
  VisionScheduleMediator,

  Count
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
