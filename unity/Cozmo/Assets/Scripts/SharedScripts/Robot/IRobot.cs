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
public delegate void ChargerStateEventHandler(ActiveObject charger);
public delegate void FaceStateEventHandler(Face face);
public delegate void PetFaceStateEventHandler(PetFace face);
public delegate void EnrolledFaceRemoved(int faceId, string faceName);
public delegate void EnrolledFaceRenamed(int faceId, string faceName);

// Interface for Robot so we can mock it in unit tests
public interface IRobot : IDisposable {

  byte ID { get; }

  /// <summary>
  /// Head angle in radians
  /// </summary>
  float HeadAngle { get; }

  float PoseAngle { get; }

  float PitchAngle { get; }

  float RollAngle { get; }

  float LeftWheelSpeed { get; }

  float RightWheelSpeed { get; }

  float LiftHeight { get; }

  float LiftHeightFactor { get; }

  Vector3 WorldPosition { get; }

  Quaternion Rotation { get; }

  Vector3 Forward { get; }

  Vector3 Right { get; }

  RobotStatusFlag RobotStatus { get; }

  OffTreadsState TreadState { get; }

  bool HasHiccups { get; }

  GameStatusFlag GameStatus { get; }

  float BatteryVoltage { get; }

  // objects that are currently visible (cubes, charger)
  Dictionary<int, ObservableObject> VisibleObjects { get; }

  // objects with poses known by blockworld
  Dictionary<int, ObservableObject> KnownObjects { get; }

  // cubes that are active and we can talk to / hear
  Dictionary<int, LightCube> LightCubes { get; }

  event LightCubeStateEventHandler OnLightCubeAdded;
  event LightCubeStateEventHandler OnLightCubeRemoved;

  Dictionary<int, LightCube> VisibleLightCubes { get; }

  ActiveObject Charger { get; }

  event ChargerStateEventHandler OnChargerAdded;
  event ChargerStateEventHandler OnChargerRemoved;

  event Action<int> OnNumBlocksConnectedChanged;
  event Action<FaceEnrollmentCompleted> OnEnrolledFaceComplete;

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

  BehaviorClass CurrentBehaviorClass { get; set; }
  ReactionTrigger CurrentReactionTrigger { get; set; }

  BehaviorID CurrentBehaviorID { get; set; }

  string CurrentBehaviorDisplayNameKey { get; set; }

  string CurrentDebugAnimationString { get; set; }

  uint FirmwareVersion { get; set; }

  uint SerialNumber { get; set; }

  BodyColor BodyColor { get; set; }

  int BodyHWVersion { get; set; }

  ObservableObject CarryingObject { get; }

  event Action<ObservableObject> OnCarryingObjectSet;

  ObservableObject HeadTrackingObject { get; }

  event Action<ObservableObject> OnHeadTrackingObjectSet;

  bool Status(RobotStatusFlag s);

  bool IsLocalized();

  Vector3 WorldToCozmo(Vector3 worldSpacePosition);

  bool IsLightCubeInPickupRange(LightCube lightCube);

  void ResetRobotState(Action onComplete = null, bool disableFreeplay = true);

  void RobotStartIdle(bool disableFreeplay = true);

  void RobotResumeFromIdle(bool freePlay);

  void TryResetHeadAndLift(Action onComplete);

  void ClearData(bool initializing = false);

  ActiveObject GetActiveObjectById(int id);

  LightCube GetLightCubeWithFactoryID(uint factoryID);

  LightCube GetLightCubeWithObjectType(ObjectType objectType);

  ActiveObject GetActiveObjectWithFactoryID(uint factoryID);

  void VisualizeQuad(Vector3 lowerLeft, Vector3 upperRight);

  void AddToEmotion(Anki.Cozmo.EmotionType type, float deltaValue, string source);

  void SetEmotion(Anki.Cozmo.EmotionType type, float value);

  void SetCalibrationData(float focalLengthX, float focalLengthY, float centerX, float centerY);

  void SetEnableCliffSensor(bool enabled);

  void RequestRandomGame();

  void DoRandomSpark();

  void SetCurrentSpark(UnlockId id);

  void EnableSparkUnlock(Anki.Cozmo.UnlockId id);

  void StopSparkUnlock();

  void SetSparkedMusicState(Anki.AudioMetaData.SwitchState.Sparked sparkedState);

  void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams);

  void DriveHead(float speed_radps);

  void MoveLift(float speed_radps);

  void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps);

  void DriveArc(float wheelSpeedMmps, short curveRadiusMm, float accelMmps = 0.0f);

  void StopAllMotors();

  uint PlaceObjectOnGroundHere(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint PlaceObjectRel(ObservableObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint PlaceOnObject(ObservableObject target, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN);

  void CancelActionByIdTag(uint idTag);

  void CancelCallback(RobotCallback callback);

  void CancelAllCallbacks();

  void SetFaceToEnroll(int existingID, string name, bool saveToRobot = true, bool sayName = true, bool useMusic = true);

  void CancelFaceEnrollment();

  uint SendAnimationTrigger(AnimationTrigger animTriggerEvent, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool useSafeLiftMotion = true, bool ignoreBodyTrack = false, bool ignoreHeadTrack = false, bool ignoreLiftTrack = false, uint loops = 1);

  void PushDrivingAnimations(AnimationTrigger drivingStartAnim, AnimationTrigger drivingLoopAnim, AnimationTrigger drivingEndAnim, string lockName);

  void RemoveDrivingAnimations(string lockName);

  void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues, bool setUnspecifiedToDefault = false);

  uint PlayNeedsGetOutAnimIfNeeded(RobotCallback callback = null);

  float GetHeadAngleFactor();

  uint SetHeadAngle(float angleFactor = -0.8f,
                      RobotCallback callback = null,
                      QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                      bool useExactAngle = false, float speed_radPerSec = -1, float accel_radPerSec2 = -1);

  void SetDefaultHeadAndLiftState(bool enable, float headAngleFactor, float liftHeight);

  void SetRobotVolume(float volume);

  float GetRobotVolume();

  void TrackToObject(ObservableObject observableObject, bool headOnly = true);

  void StopTrackToObject();

  uint TurnTowardsObject(ObservableObject observableObject, bool headTrackWhenDone = true, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f,
                           RobotCallback callback = null,
                           QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                           float setTiltTolerance_rad = 0f, float maxTurnAngle_rad = float.MaxValue);

  uint TurnTowardsFace(Face face, float maxTurnAngle_rad = Mathf.PI, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f,
                         bool sayName = false, AnimationTrigger namedTrigger = AnimationTrigger.Count,
                         AnimationTrigger unnamedTrigger = AnimationTrigger.Count,
                         RobotCallback callback = null,
                         QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint TurnTowardsLastFacePose(float maxTurnAngle, bool sayName = false, AnimationTrigger namedTrigger = AnimationTrigger.Count, AnimationTrigger unnamedTrigger = AnimationTrigger.Count, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint PickupObject(ObservableObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint RollObject(ObservableObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool checkForObjectOnTop = true, bool rollWithoutDocking = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint GotoPose(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint GotoPose(float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint DriveStraightAction(float speed_mmps, float dist_mm, bool shouldPlayAnimation = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint GotoObject(ObservableObject obj, float distance_mm, bool goToPreDockPose = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint AlignWithObject(ObservableObject obj, float distanceFromMarker_mm, RobotCallback callback = null, bool useApproachAngle = false, bool usePreDockPose = false, float approachAngleRad = 0.0f, AlignmentType alignmentType = AlignmentType.CUSTOM, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, byte numRetries = 0);

  LightCube GetClosestLightCube();

  uint SearchForCube(int cube, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint SearchForNearbyObject(int objectId = -1, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                               float backupDistance_mm = (float)SearchForNearbyObjectDefaults.BackupDistance_mm,
                               float backupSpeed_mm = (float)SearchForNearbyObjectDefaults.BackupSpeed_mms,
                               float headAngle_rad = Mathf.Deg2Rad * (float)SearchForNearbyObjectDefaults.HeadAngle_deg);

  uint SetLiftHeight(float heightFactor, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, float speed_radPerSec = -1, float accel_radPerSec2 = -1);

  void SetRobotCarryingObject(int objectID = -1);

  //  void ClearAllBlocks();
  //
  //  void ClearAllObjects();

  void VisionWhileMoving(bool enable);

  uint TurnInPlace(float angle_rad, float speed_rad_per_sec, float accel_rad_per_sec2, float tolerance_rad = 0.0f, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false);

  void SetVisionMode(VisionMode mode, bool enable);

  void RequestSetUnlock(Anki.Cozmo.UnlockId unlockID, bool unlocked);

  void ExecuteBehaviorByExecutableType(ExecutableBehaviorType type, int numTimesToExecute = -1);

  void ExecuteReactionTrigger(Anki.Cozmo.ReactionTriggerToBehavior reactionTrigger);

  void ExecuteBehaviorByID(BehaviorID behaviorID, int numTimesToExecute = -1);

  void SetEnableFreeplayActivity(bool enable);

  void RequestReactionTriggerMap();

  void RequestAllBehaviorsList();

  void ActivateHighLevelActivity(HighLevelActivity activityType);

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

  ActiveObject GetCharger();

  uint MountCharger(ActiveObject charger, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint SayTextWithEvent(string text, AnimationTrigger playEvent, SayTextIntent intent = SayTextIntent.Text, bool fitToDuration = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void UpdateEnrolledFaceByID(int faceID, string oldFaceName, string newFaceName);

  void EraseAllEnrolledFaces();

  void EraseEnrolledFaceByID(int faceID);

  void LoadFaceAlbumFromFile(string path, bool isPathRelative = true);

  // locks all reactionary behaviors under the id passed in.
  void DisableAllReactionsWithLock(string id);

  // set the locks for specific reactionary behaviors by the string id.
  void DisableReactionsWithLock(string id, AllTriggersConsidered triggerFlags);

  void RemoveDisableReactionsLock(string id);

  void EnableDroneMode(bool enable);

  void RestoreRobotFromBackup(uint backupToRestoreFrom);

  void WipeRobotGameData();

  void WipeRobotNeedsData();

  void WipeDeviceNeedsData(bool reinitializeNeeds);

  void RequestRobotRestoreData();

  void NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag tag, byte[] data, byte index = 0, byte numTotalBlobs = 1);

  uint SendQueueSingleAction<T>(T action, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, byte numRetries = 0);

  void SendQueueCompoundAction(Anki.Cozmo.ExternalInterface.RobotActionUnion[] actions, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool isParallel = false);

  void EnableCubeSleep(bool enable, bool skipAnimation = false);

  void EnableCubeLightsStateTransitionMessages(bool enable);

  void FlashCurrentLightsState(uint objectId);

  void EnableLift(bool enable);

  void EnterSDKMode(bool isExternalSdkMode);

  void ExitSDKMode(bool isExternalSdkMode);

  void SetNightVision(bool enable);

  uint PlayCubeAnimationTrigger(ObservableObject obj, CubeAnimationTrigger trigger, RobotCallback callback = null);

  uint WaitAction(float waitTime_s, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint DisplayFaceImage(uint duration_ms, byte[] faceData, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  uint DriveOffChargerContacts(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);
}