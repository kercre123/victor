using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Cozmo.Util;
using G2U = Anki.Cozmo.ExternalInterface;

public delegate void RobotCallback(bool success);
public delegate void FriendshipLevelUp(int newLevel);

public interface ILight {
  uint OnColor { get; set; }

  uint OffColor { get; set; }

  uint OnPeriodMs { get; set; }

  uint OffPeriodMs { get; set; }

  uint TransitionOnPeriodMs { get; set; }

  uint TransitionOffPeriodMs { get; set; }

  void SetLastInfo();

  bool Changed { get; }

  void ClearData();
}

public delegate void LightCubeStateEventHandler(LightCube cube);
public delegate void ChargerStateEventHandler(ObservedObject charger);
public delegate void FaceStateEventHandler(Face face);
public delegate void PetFaceStateEventHandler(PetFace face);
public delegate void EnrolledFaceRemoved(int faceId,string faceName);
public delegate void EnrolledFaceRenamed(int faceId,string faceName);

// Interface for Robot so we can mock it in unit tests
public interface IRobot : IDisposable {

  byte ID { get; }

  float HeadAngle { get; }

  float PoseAngle { get; }

  float PitchAngle { get; }

  float LeftWheelSpeed { get; }

  float RightWheelSpeed { get; }

  float LiftHeight { get; }

  float LiftHeightFactor { get; }

  Vector3 WorldPosition { get; }

  Quaternion Rotation { get; }

  Vector3 Forward { get; }

  Vector3 Right { get; }

  RobotStatusFlag RobotStatus { get; }

  GameStatusFlag GameStatus { get; }

  float BatteryVoltage { get; }

  Dictionary<int, LightCube> LightCubes { get; }

  event LightCubeStateEventHandler OnLightCubeAdded;
  event LightCubeStateEventHandler OnLightCubeRemoved;

  List<LightCube> VisibleLightCubes { get; }

  ObservedObject Charger { get; }

  event ChargerStateEventHandler OnChargerAdded;
  event ChargerStateEventHandler OnChargerRemoved;

  List<Face> Faces { get; }

  event FaceStateEventHandler OnFaceAdded;
  event FaceStateEventHandler OnFaceRemoved;
  event EnrolledFaceRemoved OnEnrolledFaceRemoved;
  event EnrolledFaceRenamed OnEnrolledFaceRenamed;

  Dictionary<int, string> EnrolledFaces { get; set; }

  Dictionary<int, float> EnrolledFacesLastEnrolledTime { get; set; }

  Dictionary<int, float> EnrolledFacesLastSeenTime { get; set; }

  event PetFaceStateEventHandler OnPetFaceAdded;
  event PetFaceStateEventHandler OnPetFaceRemoved;

  List<PetFace> PetFaces { get; }

  float[] EmotionValues { get; }

  ILight[] BackpackLights { get; }

  bool IsSparked { get; }

  Anki.Cozmo.UnlockId SparkUnlockId { get; }

  int CarryingObjectID { get; }

  int HeadTrackingObjectID { get; }

  string CurrentBehaviorString { get; set; }

  BehaviorType CurrentBehaviorType { get; set; }

  string CurrentBehaviorName { get; set; }

  string CurrentDebugAnimationString { get; set; }

  uint FirmwareVersion { get; set; }

  uint SerialNumber { get; set; }

  ObservedObject CarryingObject { get; }

  event Action<ObservedObject> OnCarryingObjectSet;

  ObservedObject HeadTrackingObject { get; }

  event Action<ObservedObject> OnHeadTrackingObjectSet;

  bool Status(RobotStatusFlag s);

  bool IsLocalized();

  Vector3 WorldToCozmo(Vector3 worldSpacePosition);

  bool IsLightCubeInPickupRange(LightCube lightCube);

  void ResetRobotState(Action onComplete = null);

  void RobotStartIdle();

  void RobotResumeFromIdle(bool freePlay);

  void TryResetHeadAndLift(Action onComplete);

  void ClearData(bool initializing = false);

  ObservedObject GetObservedObjectById(int id);

  LightCube GetLightCubeWithFactoryID(uint factoryID);

  ObservedObject GetObservedObjectWithFactoryID(uint factoryID);

  void VisualizeQuad(Vector3 lowerLeft, Vector3 upperRight);

  void AddToEmotion(Anki.Cozmo.EmotionType type, float deltaValue, string source);

  void SetEmotion(Anki.Cozmo.EmotionType type, float value);

  void SetCalibrationData(float focalLengthX, float focalLengthY, float centerX, float centerY);

  void SetEnableCliffSensor(bool enabled);

  void EnableSparkUnlock(Anki.Cozmo.UnlockId id);

  void StopSparkUnlock();

  void ActivateSparkedMusic(Anki.Cozmo.UnlockId behaviorUnlockId, Anki.Cozmo.Audio.GameState.Music musicState, Anki.Cozmo.Audio.SwitchState.Sparked sparkedState);

  void DeactivateSparkedMusic(Anki.Cozmo.UnlockId behaviorUnlockId, Anki.Cozmo.Audio.GameState.Music musicState);

  // enable/disable games available for Cozmo to request
  void SetAvailableGames(BehaviorGameFlag games);

  void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams);

  void DriveHead(float speed_radps);

  void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps);

  void DriveArc(float wheelSpeedMmps, int curveRadiusMm);

  void StopAllMotors();

  void PlaceObjectOnGroundHere(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PlaceObjectRel(ObservedObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PlaceOnObject(ObservedObject target, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN);

  void CancelCallback(RobotCallback callback);

  void CancelAllCallbacks();

  void EnrollNamedFace(int faceID, int mergeIntoID, string name, Anki.Cozmo.FaceEnrollmentSequence seq = Anki.Cozmo.FaceEnrollmentSequence.Default, bool saveToRobot = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SendAnimationTrigger(AnimationTrigger animTriggerEvent, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool useSafeLiftMotion = true);

  void SetIdleAnimation(AnimationTrigger default_anim);

  void PushIdleAnimation(AnimationTrigger default_anim);

  void PopIdleAnimation();

  void PopDrivingAnimations();

  void PushDrivingAnimations(AnimationTrigger drivingStartAnim, AnimationTrigger drivingLoopAnim, AnimationTrigger drivingEndAnim);

  void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues, bool setUnspecifiedToDefault = false);

  float GetHeadAngleFactor();

  void SetHeadAngle(float angleFactor = -0.8f,
                    RobotCallback callback = null,
                    QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                    bool useExactAngle = false, float speed_radPerSec = -1, float accel_radPerSec2 = -1);

  void SetDefaultHeadAndLiftState(bool enable, float headAngleFactor, float liftHeight);

  void SetRobotVolume(float volume);

  float GetRobotVolume();

  void TrackToObject(ObservedObject observedObject, bool headOnly = true);

  void StopTrackToObject();

  void TurnTowardsObject(ObservedObject observedObject, bool headTrackWhenDone = true, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f,
                         RobotCallback callback = null,
                         QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                         float setTiltTolerance_rad = 0f);

  void TurnTowardsFacePose(Face face, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f,
                           RobotCallback callback = null,
                           QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void TurnTowardsLastFacePose(float maxTurnAngle, bool sayName = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint PickupObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void RollObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void GotoPose(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void GotoPose(float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void DriveStraightAction(float speed_mmps, float dist_mm, bool shouldPlayAnimation = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void GotoObject(ObservedObject obj, float distance_mm, bool goToPreDockPose = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void AlignWithObject(ObservedObject obj, float distanceFromMarker_mm, RobotCallback callback = null, bool useApproachAngle = false, bool usePreDockPose = false, float approachAngleRad = 0.0f, AlignmentType alignmentType = AlignmentType.CUSTOM, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  LightCube GetClosestLightCube();

  void SearchForCube(int cube, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SearchForNearbyObject(int objectId = -1, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                             float backupDistance_mm = (float)SearchForNearbyObjectDefaults.BackupDistance_mm,
                             float backupSpeed_mm = (float)SearchForNearbyObjectDefaults.BackupSpeed_mms,
                             float headAngle_rad = Mathf.Deg2Rad * (float)SearchForNearbyObjectDefaults.HeadAngle_deg);

  void SetLiftHeight(float heightFactor, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, float speed_radPerSec = -1, float accel_radPerSec2 = -1);

  void SetRobotCarryingObject(int objectID = -1);

  void ClearAllBlocks();

  void ClearAllObjects();

  void VisionWhileMoving(bool enable);

  void TurnInPlace(float angle_rad, float speed_rad_per_sec, float accel_rad_per_sec2, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false);

  void SetVisionMode(VisionMode mode, bool enable);

  void RequestSetUnlock(Anki.Cozmo.UnlockId unlockID, bool unlocked);

  void ExecuteBehavior(BehaviorType type);

  void ExecuteBehaviorByName(string behaviorName);

  void SetEnableFreeplayBehaviorChooser(bool enable);

  void ActivateBehaviorChooser(BehaviorChooserType behaviorChooserType);

  void TurnOffBackpackLED(LEDId ledToChange);

  void SetAllBackpackBarLED(Color color);

  void SetAllBackpackBarLED(uint colorUint);

  void TurnOffAllBackpackBarLED();

  void SetBackpackBarLED(LEDId ledToChange, Color color);

  void SetBackbackArrowLED(LEDId ledId, float intensity);

  void SetFlashingBackpackLED(LEDId ledToChange, Color color, uint onDurationMs = 200, uint offDurationMs = 200, uint transitionDurationMs = 0);

  void SetEnableFreeplayLightStates(bool enable, int objectID = -1);

  void TurnOffAllLights(bool now = false);

  void UpdateLightMessages(bool now = false);

  ObservedObject GetCharger();

  void MountCharger(ObservedObject charger, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SayTextWithEvent(string text, AnimationTrigger playEvent, SayTextIntent intent = SayTextIntent.Text, bool fitToDuration = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void UpdateEnrolledFaceByID(int faceID, string oldFaceName, string newFaceName);

  void EraseAllEnrolledFaces();

  void EraseEnrolledFaceByID(int faceID);

  void LoadFaceAlbumFromFile(string path, bool isPathRelative = true);

  void EnableReactionaryBehaviors(bool enable);

  void RequestEnableReactionaryBehavior(string id, Anki.Cozmo.BehaviorType behaviorType, bool enable);

  void EnableDroneMode(bool enable);

  void RestoreRobotFromBackup(uint backupToRestoreFrom);

  void WipeRobotGameData();

  void RequestRobotRestoreData();

  void NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag tag, byte[] data, byte index = 0, byte numTotalBlobs = 1);

  uint SendQueueSingleAction<T>(T action, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SendQueueCompoundAction(Anki.Cozmo.ExternalInterface.RobotActionUnion[] actions, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool isParallel = false);

  void EnableCubeSleep(bool enable);

  void EnableCubeLightsStateTransitionMessages(bool enable);

  void FlashCurrentLightsState(uint objectId);

  void EnableLift(bool enable);

  void EnterSDKMode();

  void ExitSDKMode();

  void SetNightVision(bool enable);
}