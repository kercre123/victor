#ifndef __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
#define __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__

namespace Anki {
namespace Vector {

enum class BEIComponentID{
  AIComponent,
  Animation,
  BackpackLightComponent,
  BeatDetector,
  BehaviorContainer,
  BehaviorEvent,
  BehaviorTimerManager,
  BlockWorld,
  CliffSensor,
  CubeAccel,
  CubeComms,
  CubeConnectionCoordinator,
  CubeInteractionTracker,
  CubeLight,
  DataAccessor,
  Delegation,
  FaceWorld,
  HabitatDetector,
  HeldInPalmTracker,
  Map,
  MicComponent,
  MoodManager,
  MovementComponent,
  PetWorld,
  PhotographyManager,
  PowerStateManager,
  ProxSensor,
  PublicStateBroadcaster,
  RobotAudioClient,
  RobotInfo,
  SDK,
  SettingsCommManager,
  SettingsManager,
  SleepTracker,
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
