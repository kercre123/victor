using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;

public class MockRobot : IRobot {
  #region IRobot implementation

  private struct CallbackWrapper {
    public float CallbackTime;
    public RobotCallback Callback;
  }

  private readonly List<CallbackWrapper> _Callbacks = new List<CallbackWrapper>();

  public event System.Action<ObservableObject> OnCarryingObjectSet;

  public event System.Action<ObservableObject> OnHeadTrackingObjectSet;

  public event System.Action<int> OnNumBlocksConnectedChanged;

  public MockRobot(byte id) {
    ID = id;
    Rotation = Quaternion.identity;
    Dictionary<Anki.Cozmo.UnlockId, bool> defaultValues = new Dictionary<UnlockId, bool>();
    for (int i = 0; i < (int)Anki.Cozmo.UnlockId.Count; ++i) {
      defaultValues.Add((Anki.Cozmo.UnlockId)i, false);
    }
    UnlockablesManager.Instance.OnConnectLoad(defaultValues);
    _EnrolledFaces.Add(1, "Alice");
    _EnrolledFaces.Add(2, "Bob");
    _EnrolledFaces.Add(3, "Carol");

    _EnrolledFacesLastSeenTime.Add(1, -10);
    _EnrolledFacesLastSeenTime.Add(2, -100);
    _EnrolledFacesLastSeenTime.Add(3, -1000000);

    _EnrolledFacesLastEnrolledTime.Add(1, -9000);
    _EnrolledFacesLastEnrolledTime.Add(2, -9001);
    _EnrolledFacesLastEnrolledTime.Add(3, -3000000);

    PetFaces = new List<PetFace>();
    CurrentBehaviorDisplayNameKey = string.Empty;
  }

  public bool Status(Anki.Cozmo.RobotStatusFlag s) {
    return (RobotStatus & s) == s;
  }

  public bool IsLocalized() {
    return true;
  }

  public Vector3 WorldToCozmo(Vector3 worldSpacePosition) {
    Vector3 offset = worldSpacePosition - this.WorldPosition;
    offset = Quaternion.Inverse(this.Rotation) * offset;
    return offset;
  }

  public bool IsLightCubeInPickupRange(LightCube lightCube) {
    var bounds = new Bounds(
                   new Vector3(CozmoUtil.kOriginToLowLiftDDistMM, 0, CozmoUtil.kBlockLengthMM * 0.5f),
                   Vector3.one * CozmoUtil.kBlockLengthMM);

    return bounds.Contains(WorldToCozmo(lightCube.WorldPosition));
  }

  public void ResetRobotState(System.Action onComplete = null) {
    if (onComplete != null) {
      onComplete();
    }
  }


  public void RobotStartIdle() {
  }

  public void RobotResumeFromIdle(bool freePlay) {
  }


  public void TryResetHeadAndLift(System.Action onComplete) {
    HeadAngle = 0f;
    LiftHeight = 0f;
    if (onComplete != null) {
      onComplete();
    }
  }

  public void ClearData(bool initializing = false) {
    // Do nothing
  }

  public void UpdateCallbackTicks() {
    // This is called every frame in the Update of RobotEngineManager, so its the perfect place to tick our callbacks
    var now = Time.time;
    for (int i = _Callbacks.Count - 1; i >= 0; i--) {
      if (now > _Callbacks[i].CallbackTime) {
        var cb = _Callbacks[i].Callback;
        _Callbacks.RemoveAt(i);
        if (cb != null) {
          // everything succeeds for now
          cb(true);
        }
      }
    }
  }

  public LightCube GetLightCubeWithFactoryID(uint factoryID) {
    return null;
  }

  public ActiveObject GetActiveObjectWithFactoryID(uint factoryID) {
    return null;
  }

  public void VisualizeQuad(Vector3 lowerLeft, Vector3 upperRight) {
    // Do nothing
  }

  public void AddToEmotion(Anki.Cozmo.EmotionType type, float deltaValue, string source) {
    _EmotionValues[(int)type] += deltaValue;
  }

  public void SetEmotion(Anki.Cozmo.EmotionType type, float value) {
    _EmotionValues[(int)type] = value;
  }

  public void SetCalibrationData(float focalLengthX, float focalLengthY, float centerX, float centerY) {

  }

  public void SetEnableCliffSensor(bool enabled) {
    // Do nothing
  }

  public void EnableSparkUnlock(Anki.Cozmo.UnlockId id) {
    IsSparked = (id != UnlockId.Count);
    SparkUnlockId = id;
  }

  public void StopSparkUnlock() {
    EnableSparkUnlock(UnlockId.Count);
  }

  public void ActivateSparkedMusic(Anki.Cozmo.UnlockId behaviorUnlockId, Anki.Cozmo.Audio.GameState.Music musicState, Anki.Cozmo.Audio.SwitchState.Sparked sparkedState) {
    // Do nothing
  }

  public void DeactivateSparkedMusic(Anki.Cozmo.UnlockId behaviorUnlockId, Anki.Cozmo.Audio.GameState.Music musicState) {
    // Do nothing
  }

  // enable/disable games available for Cozmo to request
  public void SetAvailableGames(BehaviorGameFlag games) {

  }

  public ActiveObject GetActiveObjectById(int id) {
    return null;
  }

  public void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams) {
  }

  public void DriveHead(float speed_radps) {
  }

  public void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps) {
    LeftWheelSpeed = leftWheelSpeedMmps;
    RightWheelSpeed = rightWheelSpeedMmps;
  }

  public void DriveArc(float wheelSpeedMmps, int curveRadiusMm) {
  }

  public void StopAllMotors() {
    LeftWheelSpeed = 0;
    RightWheelSpeed = 0;
  }

  private void QueueCallback(float delay, RobotCallback callback) {
    _Callbacks.Add(new CallbackWrapper() { CallbackTime = Time.time + delay, Callback = callback });
  }

  public void PlaceObjectOnGroundHere(RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    if (CarryingObject == null) {
      // Can't place object if carrying object is null
      if (callback != null) {
        callback(false);
        return;
      }
    }

    CarryingObject.SetPosition(WorldPosition + Rotation * (Vector3.right * CozmoUtil.kOriginToLowLiftDDistMM));
    CarryingObject.SetRotation(Rotation);

    CarryingObject = null;
    LiftHeight = 0f;

    QueueCallback(0.5f, callback);
  }

  public void PlaceObjectRel(ObservableObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    if (CarryingObject == null) {
      // Can't place object if carrying object is null
      if (callback != null) {
        callback(false);
        return;
      }
    }

    Rotation = Quaternion.Euler(0, 0, approachAngle);
    CarryingObject.SetPosition(WorldPosition + Rotation * (Vector3.left * offsetFromMarker));
    CarryingObject.SetRotation(Rotation);

    WorldPosition = (CarryingObject.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();

    CarryingObject = null;
    LiftHeight = 0f;

    QueueCallback(1f, callback);
  }

  public void PlaceOnObject(ObservableObject target, bool checkForObjectOnTop = true, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    if (CarryingObject == null) {
      // Can't place object if carrying object is null
      if (callback != null) {
        callback(false);
        return;
      }
    }

    CarryingObject.SetPosition(target.WorldPosition + Vector3.forward * CozmoUtil.kBlockLengthMM);
    CarryingObject.SetRotation(Rotation);
    WorldPosition = (target.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();

    CarryingObject = null;
    LiftHeight = 0f;

    QueueCallback(1f, callback);
  }

  public void CancelAction(Anki.Cozmo.RobotActionType actionType = Anki.Cozmo.RobotActionType.UNKNOWN) {
    // TODO: Something?
  }

  public void CancelCallback(RobotCallback callback) {
    for (int i = _Callbacks.Count - 1; i >= 0; i--) {
      if (_Callbacks[i].Callback == callback) {
        _Callbacks.RemoveAt(i);
      }
    }
  }

  public void CancelAllCallbacks() {
    _Callbacks.Clear();
  }

  public void SetFaceToEnroll(int existingID, string name, bool saveToRobot = true, bool sayName = true, bool useMusic = true) {
    // Do nothing
  }

  public void CancelFaceEnrollment() {
    // Do nothing
  }
  
  public void SendAnimationTrigger(AnimationTrigger animTriggerEvent, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool useSafeLiftMotion = true, bool ignoreBodyTrack = false, bool ignoreHeadTrack = false, bool ignoreLiftTrack = false) {
    QueueCallback(0.5f, callback);
  }

  public void SetIdleAnimation(AnimationTrigger default_anim) {
    // Do nothing
  }

  public void PushIdleAnimation(AnimationTrigger default_anim) {
    // Do nothing
  }

  public void PopIdleAnimation() {
    // Do nothing
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues, bool setUnspecifiedToDefault = false) {
    // Do nothing
  }

  public void PopDrivingAnimations() {
    // Do nothing
  }

  public void PushDrivingAnimations(AnimationTrigger drivingStartAnim, AnimationTrigger drivingLoopAnim, AnimationTrigger drivingEndAnim) {
    // Do nothing
  }

  public float GetHeadAngleFactor() {
    float angle = HeadAngle;

    if (angle >= 0f) {
      angle = Mathf.Lerp(0f, 1f, angle / (CozmoUtil.kMaxHeadAngle * Mathf.Deg2Rad));
    }
    else {
      angle = Mathf.Lerp(0f, -1f, angle / (CozmoUtil.kMinHeadAngle * Mathf.Deg2Rad));
    }

    return angle;
  }

  public void SetHeadAngle(float angleFactor = -0.8f, RobotCallback callback = null,
                           Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW, bool useExactAngle = false, float speed_radPerSec = -1, float accel_radPerSec2 = -1) {
    float radians = angleFactor;

    if (!useExactAngle) {
      if (angleFactor >= 0f) {
        radians = Mathf.Lerp(0f, CozmoUtil.kMaxHeadAngle * Mathf.Deg2Rad, angleFactor);
      }
      else {
        radians = Mathf.Lerp(0f, CozmoUtil.kMinHeadAngle * Mathf.Deg2Rad, -angleFactor);
      }
    }

    if (HeadTrackingObject == -1 && Mathf.Abs(radians - HeadAngle) < 0.001f && queueActionPosition == QueueActionPosition.NOW) {
      if (callback != null) {
        callback(true);
      }
      return;
    }
    HeadAngle = radians;

    QueueCallback(0.1f, callback);
  }


  public void SetDefaultHeadAndLiftState(bool enable, float headAngleFactor, float liftHeight) {
    if (enable) {
      HeadAngle = headAngleFactor;
      LiftHeight = liftHeight;
    }
  }

  private float _RobotVolume = 0f;

  public void SetRobotVolume(float volume) {
    _RobotVolume = volume;
  }

  public float GetRobotVolume() {
    return _RobotVolume;
  }

  public void TrackToObject(ObservableObject observedObject, bool headOnly = true) {
    // TODO: Something?
    HeadTrackingObject = observedObject;
  }

  public ActiveObject GetCharger() {
    return null;
  }

  public void MountCharger(ActiveObject charger, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

  }

  public void StopTrackToObject() {
    // TODO: Something?
    HeadTrackingObject = null;
  }

  private void LookAtPosition(Vector3 pos) {
    var positionDelta = pos - WorldPosition;
    var rotationDelta = new Vector3(positionDelta.x, positionDelta.y, 0);

    var headDelta = new Vector2(positionDelta.x, positionDelta.z - CozmoUtil.kHeadHeightMM);
    var rotation = Quaternion.LookRotation(rotationDelta, Vector3.up);

    HeadAngle = Mathf.Atan2(headDelta.y, headDelta.x);
    Rotation = rotation;
  }

  public void TurnTowardsObject(ObservableObject observableObject, bool headTrackWhenDone = true, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW,
                                float setTiltTolerance_rad = 0f) {

    LookAtPosition(observableObject.WorldPosition);

    if (headTrackWhenDone) {
      TrackToObject(observableObject);
    }
    QueueCallback(1f, callback);
  }

  public void TurnTowardsFace(Face face, float maxTurnAngle_rad = Mathf.PI, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f,
                              bool sayName = false, AnimationTrigger namedTrigger = AnimationTrigger.Count,
                              AnimationTrigger unnamedTrigger = AnimationTrigger.Count,
                              RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    LookAtPosition(face.WorldPosition);

    QueueCallback(1f, callback);
  }

  // Turns towards the last seen face, but not any more than the specified maxTurnAngle
  public void TurnTowardsLastFacePose(float maxTurnAngle, bool sayName = false, AnimationTrigger namedTrigger = AnimationTrigger.Count,
                                      AnimationTrigger unnamedTrigger = AnimationTrigger.Count, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    DAS.Debug(this, "TurnTowardsLastFacePose with maxTurnAngle : " + maxTurnAngle);

    TurnInPlace(maxTurnAngle, 4.3f, 10f);

    QueueCallback(1f, callback);
  }

  public uint PickupObject(ObservableObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0f, bool checkForObjectOnTop = true, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    CarryingObject = selectedObject;
    LiftHeight = 1f;

    if (selectedObject != null) {
      Rotation = CarryingObject.Rotation.zRotation();
      CarryingObject.SetPosition(new Vector3(CarryingObject.WorldPosition.x, CarryingObject.WorldPosition.y, CozmoUtil.kCarriedObjectHeight));
      WorldPosition = (CarryingObject.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();
    }

    QueueCallback(5f, callback);
    return 0;
  }

  public void RollObject(ObservableObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool checkForObjectOnTop = true, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    // Rather than figure out what side the cube was on, assume it just goes upright
    Rotation = selectedObject.Rotation.zRotation();
    selectedObject.SetRotation(Rotation);
    WorldPosition = (selectedObject.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();

    QueueCallback(3f, callback);
  }

  public void PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    if (CarryingObject == null) {
      // Can't place object if carrying object is null
      if (callback != null) {
        callback(false);
        return;
      }
    }

    CarryingObject.SetRotation(rotation);
    CarryingObject.SetPosition(position);

    Rotation = rotation;
    WorldPosition = (CarryingObject.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();


    CarryingObject = null;
    LiftHeight = 0f;

    QueueCallback(0.5f, callback);
  }

  public void GotoPose(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    WorldPosition = position.xy0();
    Rotation = rotation;

    if (CarryingObject != null) {
      CarryingObject.SetPosition(WorldPosition + Rotation * (Vector3.right * CozmoUtil.kOriginToLowLiftDDistMM));
      CarryingObject.SetRotation(rotation);

    }

    QueueCallback(2f, callback);
  }

  public void GotoPose(float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    GotoPose(new Vector3(x_mm, y_mm, 0f), Quaternion.Euler(0, 0, Mathf.Rad2Deg * rad), level, useManualSpeed, callback, queueActionPosition);
  }

  public void DriveStraightAction(float speed_mmps, float dist_mm, bool shouldPlayAnimation = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    QueueCallback(2f, callback);
  }

  public void GotoObject(ObservableObject obj, float distance_mm, bool goToPreDockPose, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    var delta = (WorldPosition - obj.WorldPosition).normalized * distance_mm;

    WorldPosition = (obj.WorldPosition + delta).xy0();

    QueueCallback(2f, callback);
  }

  public void AlignWithObject(ObservableObject obj, float distanceFromMarker_mm, RobotCallback callback = null, bool useApproachAngle = false, bool usePreDockPose = false, float approachAngleRad = 0f, Anki.Cozmo.AlignmentType alignmentType = Anki.Cozmo.AlignmentType.CUSTOM, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    Rotation = obj.Rotation.zRotation();
    WorldPosition = (obj.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();

    QueueCallback(3f, callback);
  }

  public LightCube GetClosestLightCube() {
    return null;
  }

  public void SearchForCube(int cube, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    QueueCallback(3f, callback);
  }

  public void SearchForNearbyObject(int objectId = -1, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                                    float backupDistance_mm = (float)SearchForNearbyObjectDefaults.BackupDistance_mm,
                                    float backupSpeed_mm = (float)SearchForNearbyObjectDefaults.BackupSpeed_mms,
                                    float headAngle_rad = Mathf.Deg2Rad * (float)SearchForNearbyObjectDefaults.HeadAngle_deg) {
    QueueCallback(3f, callback);
  }

  public void SetLiftHeight(float heightFactor, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW, float speed_radPerSec = -1, float accel_radPerSec2 = -1) {
    LiftHeight = heightFactor;

    QueueCallback(0.5f, callback);
  }

  public void SetRobotCarryingObject(int objectID = -1) {
    // this one is called by a message, so ignore it
  }

  public void ClearAllBlocks() {
    // Do nothing
  }

  public void ClearAllObjects() {
    // Do nothing
  }

  public void VisionWhileMoving(bool enable) {
    // Do nothing
  }

  public void TurnInPlace(float angle_rad, float speed_rad_per_sec, float accel_rad_per_sec2, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    Rotation *= Quaternion.Euler(0, 0, angle_rad);

    QueueCallback(1f, callback);
  }

  public void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false) {
    // Do something?
  }

  public void SetVisionMode(Anki.Cozmo.VisionMode mode, bool enable) {
    // Do nothing
  }

  public void RequestSetUnlock(Anki.Cozmo.UnlockId unlockID, bool unlocked) {
    MessageEngineToGame message = new MessageEngineToGame();

    Anki.Cozmo.ExternalInterface.RequestSetUnlockResult resultMsg = new RequestSetUnlockResult();
    resultMsg.unlockID = unlockID;
    resultMsg.unlocked = unlocked;

    message.RequestSetUnlockResult = resultMsg;

    RobotEngineManager.Instance.MockCallback(message, 0.5f);
  }

  public void ExecuteBehavior(Anki.Cozmo.BehaviorType type) {
    // Do nothing
  }

  public void ExecuteBehaviorByName(string behaviorName) {
    // Do nothing
  }

  public void SetEnableFreeplayBehaviorChooser(bool enable) {
    // Do nothing
  }

  public void ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType behaviorChooserType) {
    // Do nothing
  }

  public void TurnOffBackpackLED(LEDId ledToChange) {
    SetBackpackLED((int)ledToChange, 0);
  }

  public void SetAllBackpackBarLED(Color color) {
    SetBackpackBarLED(LEDId.LED_BACKPACK_BACK, color);
    SetBackpackBarLED(LEDId.LED_BACKPACK_MIDDLE, color);
    SetBackpackBarLED(LEDId.LED_BACKPACK_FRONT, color);
  }

  public void SetAllBackpackBarLED(uint colorUint) {
    SetBackpackLED((int)LEDId.LED_BACKPACK_BACK, colorUint);
    SetBackpackLED((int)LEDId.LED_BACKPACK_MIDDLE, colorUint);
    SetBackpackLED((int)LEDId.LED_BACKPACK_FRONT, colorUint);
  }

  public void TurnOffAllBackpackBarLED() {
    SetBackpackLED((int)LEDId.LED_BACKPACK_BACK, 0);
    SetBackpackLED((int)LEDId.LED_BACKPACK_MIDDLE, 0);
    SetBackpackLED((int)LEDId.LED_BACKPACK_FRONT, 0);
  }

  public void SetBackpackBarLED(LEDId ledToChange, Color color) {
    if (ledToChange == LEDId.LED_BACKPACK_LEFT || ledToChange == LEDId.LED_BACKPACK_RIGHT) {
      DAS.Warn("Robot", "BackpackLighting - LEDId.LED_BACKPACK_LEFT or LEDId.LED_BACKPACK_RIGHT " +
      "does not have the full range of color. Taking the highest RGB value to use as the light intensity.");
      float highestIntensity = Mathf.Max(color.r, Mathf.Max(color.g, color.b));
      SetBackbackArrowLED(ledToChange, highestIntensity);
    }
    else {
      uint colorUint = color.ToUInt();
      SetBackpackLED((int)ledToChange, colorUint);
    }
  }

  public void SetBackbackArrowLED(LEDId ledId, float intensity) {
    intensity = Mathf.Clamp(intensity, 0f, 1f);
    Color color = new Color(intensity, intensity, intensity);
    uint colorUint = color.ToUInt();
    SetBackpackLED((int)ledId, colorUint);
  }

  public void SetFlashingBackpackLED(LEDId ledToChange, Color color, uint onDurationMs = 200, uint offDurationMs = 200, uint transitionDurationMs = 0) {
    uint colorUint = color.ToUInt();
    SetBackpackLED((int)ledToChange, colorUint, 0, onDurationMs, offDurationMs, transitionDurationMs, transitionDurationMs);
  }

  private void SetBackpackLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Robot.Light.FOREVER, uint offPeriod_ms = 0,
                               uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {
    for (int i = 0; i < BackpackLights.Length; ++i) {
      SetBackpackLED(i, onColor, offColor, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms);
    }
  }

  private void SetBackpackLED(int index, uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Robot.Light.FOREVER, uint offPeriod_ms = 0,
                              uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {
    // Special case for arrow lights; they only accept red as a color
    if (index == (int)LEDId.LED_BACKPACK_LEFT || index == (int)LEDId.LED_BACKPACK_RIGHT) {
      //uint whiteUint = Color.white.ToUInt();
      //onColor = (onColor > 0) ? whiteUint : 0;
      //offColor = (offColor > 0) ? whiteUint : 0;
    }

    BackpackLights[index].OnColor = onColor;
    BackpackLights[index].OffColor = offColor;
    BackpackLights[index].OnPeriodMs = onPeriod_ms;
    BackpackLights[index].OffPeriodMs = offPeriod_ms;
    BackpackLights[index].TransitionOnPeriodMs = transitionOnPeriod_ms;
    BackpackLights[index].TransitionOffPeriodMs = transitionOffPeriod_ms;

    if (index == (int)LEDId.LED_BACKPACK_RIGHT) {

    }
  }

  public void SetEnableFreeplayLightStates(bool enable, int objectID) {
  }

  public void TurnOffAllLights(bool now = false) {
    var enumerator = LightCubes.GetEnumerator();

    while (enumerator.MoveNext()) {
      LightCube lightCube = enumerator.Current.Value;

      lightCube.SetLEDs();
    }

    SetBackpackLEDs();
  }

  public void UpdateLightMessages(bool now = false) {
    // Do nothing
  }



  public byte ID {
    get;
    private set;
  }

  public float HeadAngle {
    get;
    private set;
  }

  public float PoseAngle {
    get { return Rotation.eulerAngles.z * Mathf.Deg2Rad; }
  }

  public float PitchAngle {
    get { return Rotation.eulerAngles.y * Mathf.Deg2Rad; }
  }

  public float LeftWheelSpeed {
    get;
    private set;
  }

  public float RightWheelSpeed {
    get;
    private set;
  }

  public float LiftHeight {
    get;
    private set;
  }

  public float LiftHeightFactor {
    get;
    private set;
  }

  public Vector3 WorldPosition {
    get;
    private set;
  }

  public Quaternion Rotation {
    get;
    private set;
  }

  public Vector3 Forward {
    get { return Rotation * Vector3.forward; }
  }

  public Vector3 Right {
    get { return Rotation * Vector3.right; }
  }

  public Anki.Cozmo.RobotStatusFlag RobotStatus {
    get;
    private set;
  }

  public OffTreadsState TreadState {
    get;
    private set;
  }

  public Anki.Cozmo.GameStatusFlag GameStatus {
    get;
    private set;
  }

  public float BatteryVoltage {
    get { return 4.73f; }
  }

  Dictionary<int, ObservableObject> _VisibleObjects = new Dictionary<int, ObservableObject>();
  // objects that are currently visible (cubes, charger)
  public Dictionary<int, ObservableObject> VisibleObjects {
    get {
      return _VisibleObjects;
    }
  }

  Dictionary<int, ObservableObject> _KnownObjects = new Dictionary<int, ObservableObject>();
  // objects with poses known by blockworld
  public Dictionary<int, ObservableObject> KnownObjects {
    get {
      return _KnownObjects;
    }
  }

  private readonly Dictionary<int, LightCube> _LightCubes = new Dictionary<int, LightCube>();

  public Dictionary<int, LightCube> LightCubes {
    get {
      return _LightCubes;
    }
  }

  public Dictionary<int, LightCube> VisibleLightCubes {
    get {
      Dictionary<int, LightCube> visibleCubes = new Dictionary<int, LightCube>(LightCubes);
      return visibleCubes;
    }
  }

  private readonly List<Face> _Faces = new List<Face>();

  public List<Face> Faces {
    get {
      return _Faces;
    }
  }

  private Dictionary<int, string> _EnrolledFaces = new Dictionary<int, string>();

  public Dictionary<int, string> EnrolledFaces {
    get {
      return _EnrolledFaces;
    }
    set {
      _EnrolledFaces = value;
    }
  }

  private Dictionary<int, float> _EnrolledFacesLastEnrolledTime = new Dictionary<int, float>();
  private Dictionary<int, float> _EnrolledFacesLastSeenTime = new Dictionary<int, float>();

  public Dictionary<int, float> EnrolledFacesLastEnrolledTime {
    get {
      return _EnrolledFacesLastEnrolledTime;
    }
    set {
      _EnrolledFacesLastEnrolledTime = value;
    }
  }

  public Dictionary<int, float> EnrolledFacesLastSeenTime {
    get {
      return _EnrolledFacesLastSeenTime;
    }
    set {
      _EnrolledFacesLastSeenTime = value;
    }
  }

  public int FriendshipPoints {
    get;
    private set;
  }

  public int FriendshipLevel {
    get;
    private set;
  }

  private readonly float[] _EmotionValues = new float[(int)EmotionType.Count];

  public float[] EmotionValues {
    get {
      return _EmotionValues;
    }
  }

  // There are currently 5 backpack lights
  private readonly ILight[] _BackpackLights = new ILight[] {
    new Robot.Light(),
    new Robot.Light(),
    new Robot.Light(),
    new Robot.Light(),
    new Robot.Light()
  };

  public ILight[] BackpackLights {
    get {
      return _BackpackLights;
    }
  }

  public bool IsSparked { get; private set; }

  public Anki.Cozmo.UnlockId SparkUnlockId { get; private set; }

  public ActiveObject TargetLockedObject {
    get;
    set;
  }

  public int CarryingObjectID {
    get {
      return CarryingObject != null ? CarryingObject.ID : -1;
    }
  }

  public int HeadTrackingObjectID {
    get {
      return HeadTrackingObject != null ? HeadTrackingObject.ID : -1;
    }
  }

  public string CurrentBehaviorString {
    get;
    set;
  }
  public string CurrentBehaviorDisplayNameKey { get; set; }

  public bool PlayingReactionaryBehavior {
    get; set;
  }

  public BehaviorType CurrentBehaviorType { get; set; }

  public string CurrentBehaviorName { get; set; }

  public string CurrentDebugAnimationString {
    get;
    set;
  }

  public uint FirmwareVersion {
    get;
    set;
  }

  public uint SerialNumber {
    get;
    set;
  }

  private ObservableObject _CarryingObject;

  public ObservableObject CarryingObject {
    get { return _CarryingObject; }
    private set {
      _CarryingObject = value;
      if (OnCarryingObjectSet != null) {
        OnCarryingObjectSet(_CarryingObject);
      }
    }
  }

  private ObservableObject _HeadTrackingObject;

  public ObservableObject HeadTrackingObject {
    get { return _HeadTrackingObject; }
    private set {
      _HeadTrackingObject = value;
      if (OnHeadTrackingObjectSet != null) {
        OnHeadTrackingObjectSet(_HeadTrackingObject);
      }
    }
  }

  #endregion

  #region IDisposable implementation

  public void Dispose() {
  }

  #endregion

  public void SayTextWithEvent(string text, AnimationTrigger playEvent, SayTextIntent intent = SayTextIntent.Text, bool fitToDuration = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

  }

  public void EraseAllEnrolledFaces() {

  }

  public void EraseEnrolledFaceByID(int faceID) {
    string faceName = EnrolledFaces[faceID];
    EnrolledFaces.Remove(faceID);
    EnrolledFacesLastEnrolledTime.Remove(faceID);
    EnrolledFacesLastSeenTime.Remove(faceID);
    if (OnEnrolledFaceRemoved != null) {
      OnEnrolledFaceRemoved(faceID, faceName);
    }
  }

  public void UpdateEnrolledFaceByID(int faceID, string oldFaceName, string newFaceName) {
    EnrolledFaces[faceID] = newFaceName;
    if (OnEnrolledFaceRenamed != null) {
      OnEnrolledFaceRenamed(faceID, newFaceName);
    }
  }

  public void LoadFaceAlbumFromFile(string path, bool isPathRelative = true) {

  }

  public void EnableReactionaryBehaviors(bool enable) {

  }

  public void RequestEnableReactionaryBehavior(string id, Anki.Cozmo.BehaviorType behaviorType, bool enable) {

  }

  public void EnableDroneMode(bool enable) {
  }

  public void RestoreRobotFromBackup(uint backupToRestoreFrom) {
  }

  public void WipeRobotGameData() {
  }

  public void RequestRobotRestoreData() {
  }

  public void NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag tag, byte[] data, byte index, byte numTotalBlobs) {
  }

  public uint SendQueueSingleAction<T>(T action, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return 0;
  }

  public void SendQueueCompoundAction(Anki.Cozmo.ExternalInterface.RobotActionUnion[] actions, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool isParallel = false) {
    if (callback != null) {
      QueueCallback(0.5f, callback);
    }
  }

  public void EnableCubeSleep(bool enable) {
  }

  public void EnableCubeLightsStateTransitionMessages(bool enable) {
  }

  public void FlashCurrentLightsState(uint objectId) {
  }

  public void EnableLift(bool enable) {
  }

  public void EnterSDKMode() {
  }

  public void ExitSDKMode() {
  }

  public event LightCubeStateEventHandler OnLightCubeAdded;
  public event LightCubeStateEventHandler OnLightCubeRemoved;

  public ActiveObject Charger { get; set; }

  public event ChargerStateEventHandler OnChargerAdded;
  public event ChargerStateEventHandler OnChargerRemoved;

  public event FaceStateEventHandler OnFaceAdded;
  public event FaceStateEventHandler OnFaceRemoved;
  public event EnrolledFaceRemoved OnEnrolledFaceRemoved;
  public event EnrolledFaceRenamed OnEnrolledFaceRenamed;

  public event PetFaceStateEventHandler OnPetFaceAdded;
  public event PetFaceStateEventHandler OnPetFaceRemoved;

  public List<PetFace> PetFaces { get; private set; }

  public void SetNightVision(bool enable) {
  }
}
