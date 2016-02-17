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
  uint OnColor{ get; set; }

  uint OffColor{ get; set; }

  uint OnPeriodMs{ get; set; }

  uint OffPeriodMs{ get; set; }

  uint TransitionOnPeriodMs{ get; set; }

  uint TransitionOffPeriodMs{ get; set; }

  void SetLastInfo();

  bool Changed { get; }

  void ClearData();
}

// Interface for Robot so we can mock it in unit tests
public interface IRobot : IDisposable {

  event FriendshipLevelUp OnFriendshipLevelUp;

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

  float BatteryPercent { get; }

  List<ObservedObject> VisibleObjects { get; }

  List<ObservedObject> SeenObjects { get; }

  List<ObservedObject> DirtyObjects { get; }

  Dictionary<int, LightCube> LightCubes { get; }

  List<Face> Faces { get; }

  int FriendshipPoints { get; }

  int FriendshipLevel { get; }

  float[] EmotionValues { get; }

  ILight[] BackpackLights { get; }

  ObservedObject TargetLockedObject { get; set; }

  int CarryingObjectID { get; }

  int HeadTrackingObjectID { get; }

  string CurrentBehaviorString { get; set; }

  ObservedObject CarryingObject { get; }

  event Action<ObservedObject> OnCarryingObjectSet;

  ObservedObject HeadTrackingObject { get; }

  event Action<ObservedObject> OnHeadTrackingObjectSet;

  void SetLocalBusyTimer(float localBusyTimer);

  bool IsBusy { get; }

  bool Status(RobotStatusFlag s);

  bool IsLocalized();

  void CooldownTimers(float delta);

  Vector3 WorldToCozmo(Vector3 worldSpacePosition);

  bool IsLightCubeInPickupRange(LightCube lightCube);

  void ResetRobotState(Action onComplete);

  void TryResetHeadAndLift(Action onComplete);

  void ClearData(bool initializing = false);

  void ClearVisibleObjects();

  void UpdateInfo(G2U.RobotState message);

  void UpdateDirtyList(ObservedObject dirty);

  void InitializeFriendshipPoints();

  void AddToFriendshipPoints(int deltaValue);

  string GetFriendshipLevelName(int friendshipLevel);

  int GetFriendshipLevelByName(string friendshipName);

  int GetProgressionStat(Anki.Cozmo.ProgressionStatType index);

  StatContainer GetProgressionStats();

  void AddToProgressionStat(Anki.Cozmo.ProgressionStatType index, int deltaValue);

  void VisualizeQuad(Vector3 lowerLeft, Vector3 upperRight);

  void AddToEmotion(Anki.Cozmo.EmotionType type, float deltaValue, string source);

  void SetEmotion(Anki.Cozmo.EmotionType type, float value);

  void SetProgressionStat(Anki.Cozmo.ProgressionStatType type, int value);

  void SetProgressionStats(StatContainer stats);

  void SetEnableAllBehaviors(bool enabled);

  void SetEnableBehaviorGroup(BehaviorGroup behaviorGroup, bool enabled);

  void SetEnableBehavior(string behaviorName, bool enabled);

  void ClearAllBehaviorScoreOverrides();

  void OverrideBehaviorScore(string behaviorName, float newScore);

  void UpdateObservedObjectInfo(G2U.RobotObservedObject message);

  void UpdateObservedFaceInfo(G2U.RobotObservedFace message);

  void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams);

  void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps);

  void PlaceObjectOnGroundHere(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PlaceObjectRel(ObservedObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PlaceOnObject(ObservedObject target, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN);

  void CancelCallback(RobotCallback callback);

  void CancelAllCallbacks();

  void SendAnimation(string animName, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SendAnimationGroup(string animGroupName, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SetIdleAnimation(string default_anim);

  void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues, bool setUnspecifiedToDefault = false);

  float GetHeadAngleFactor();

  void SetHeadAngle(float angleFactor = -0.8f, 
                    RobotCallback callback = null,
                    QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                    bool useExactAngle = false, 
                    float accelRadSec = 2f, 
                    float maxSpeedFactor = 1f);

  void SetRobotVolume(float volume);

  float GetRobotVolume();

  void TrackToObject(ObservedObject observedObject, bool headOnly = true);

  void StopTrackToObject();

  void FaceObject(ObservedObject observedObject, bool headTrackWhenDone = true, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f,
                  RobotCallback callback = null,
                  QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void FacePose(Face face, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f, 
                RobotCallback callback = null,
                QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PickupObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void RollObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void GotoPose(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void GotoPose(float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void GotoObject(ObservedObject obj, float distance_mm, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void AlignWithObject(ObservedObject obj, float distanceFromMarker_mm, RobotCallback callback = null, bool useApproachAngle = false, float approachAngleRad = 0.0f, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SetLiftHeight(float heightFactor, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void SetRobotCarryingObject(int objectID = -1);

  void ClearAllBlocks();

  void ClearAllObjects();

  void VisionWhileMoving(bool enable);

  void TurnInPlace(float angle_rad, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

  void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false);

  void SetVisionMode(VisionMode mode, bool enable);

  void ExecuteBehavior(BehaviorType type);

  void SetBehaviorSystem(bool enable);

  void ActivateBehaviorChooser(BehaviorChooserType behaviorChooserType);

  void TurnOffBackpackLED(LEDId ledToChange);

  void SetAllBackpackBarLED(Color color);

  void SetAllBackpackBarLED(uint colorUint);

  void TurnOffAllBackpackBarLED();

  void SetBackpackBarLED(LEDId ledToChange, Color color);

  void SetBackbackArrowLED(LEDId ledId, float intensity);

  void SetFlashingBackpackLED(LEDId ledToChange, Color color, uint onDurationMs = 200, uint offDurationMs = 200, uint transitionDurationMs = 0);

  void TurnOffAllLights(bool now = false);

  void UpdateLightMessages(bool now = false);

  ObservedObject GetCharger();

  void MountCharger(ObservedObject charger, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW);

}