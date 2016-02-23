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

  public event FriendshipLevelUp OnFriendshipLevelUp;

  public event System.Action<ObservedObject> OnCarryingObjectSet;

  public event System.Action<ObservedObject> OnHeadTrackingObjectSet;

  public MockRobot(byte id) {
    ID = id;
    Rotation = Quaternion.identity;
  }

  public void SetLocalBusyTimer(float localBusyTimer) {
    // Do nothing
  }

  public bool Status(Anki.Cozmo.RobotStatusFlag s) {
    return (RobotStatus & s) == s;
  }

  public bool IsLocalized() {
    return true;
  }

  public void CooldownTimers(float delta) {
    // Do nothing
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

  public void ResetRobotState(System.Action onComplete) {
    // Do Something?
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

  public void ClearVisibleObjects() {
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

  public void UpdateInfo(Anki.Cozmo.ExternalInterface.RobotState message) {
    // won't get called
  }

  public void UpdateDirtyList(ObservedObject dirty) {
    // won't get called
  }

  public void InitializeFriendshipPoints() {
    // Set FriendshipPoints and Level from Save Data.
    FriendshipLevel = DataPersistence.DataPersistenceManager.Instance.Data.FriendshipLevel;
    FriendshipPoints = DataPersistence.DataPersistenceManager.Instance.Data.FriendshipPoints;

    // Now initialize progress stats
    var currentSession = DataPersistence.DataPersistenceManager.Instance.CurrentSession;

    if (currentSession != null) {
      SetProgressionStats(currentSession.Progress);
    }
  }

  public void AddToFriendshipPoints(int deltaValue) {
    FriendshipPoints += deltaValue;
    ComputeFriendshipLevel();
  }

  public string GetFriendshipLevelName(int friendshipLevel) {
    FriendshipProgressionConfig levelConfig = DailyGoalManager.Instance.GetFriendshipProgressConfig();
    return levelConfig.FriendshipLevels[friendshipLevel].FriendshipLevelName;
  }

  public int GetFriendshipLevelByName(string friendshipName) {
    FriendshipProgressionConfig levelConfig = DailyGoalManager.Instance.GetFriendshipProgressConfig();
    for (int i = 0; i < levelConfig.FriendshipLevels.Length; ++i) {
      if (levelConfig.FriendshipLevels[i].FriendshipLevelName == friendshipName) {
        return i;
      }
    }
    return -1;
  }

  private void ComputeFriendshipLevel() {
    FriendshipProgressionConfig levelConfig = DailyGoalManager.Instance.GetFriendshipProgressConfig();
    bool friendshipLevelChanged = false;
    while (FriendshipLevel + 1 < levelConfig.FriendshipLevels.Length &&
           levelConfig.FriendshipLevels[FriendshipLevel + 1].PointsRequired <= FriendshipPoints) {
      FriendshipPoints -= levelConfig.FriendshipLevels[FriendshipLevel + 1].PointsRequired;
      FriendshipLevel++;
      friendshipLevelChanged = true;
    }
    if (friendshipLevelChanged) {
      if (OnFriendshipLevelUp != null) {
        OnFriendshipLevelUp(FriendshipLevel);
      }
    }
  }

  private readonly StatContainer _ProgressionStats = new StatContainer();

  public int GetProgressionStat(Anki.Cozmo.ProgressionStatType index) {
    return _ProgressionStats[index];
  }

  public StatContainer GetProgressionStats() {
    return _ProgressionStats;
  }

  public void AddToProgressionStat(Anki.Cozmo.ProgressionStatType index, int deltaValue) {
    _ProgressionStats[index] += deltaValue;
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

  public void SetProgressionStat(Anki.Cozmo.ProgressionStatType type, int value) {
    _ProgressionStats[type] = value;
  }

  public void SetProgressionStats(StatContainer stats) {
    _ProgressionStats.Set(stats);
  }

  public void SetEnableAllBehaviors(bool enabled) {
    // Do nothing
  }

  public void SetEnableBehaviorGroup(Anki.Cozmo.BehaviorGroup behaviorGroup, bool enabled) {
    // Do nothing
  }

  public void SetEnableBehavior(string behaviorName, bool enabled) {
    // Do nothing
  }

  public void ClearAllBehaviorScoreOverrides() {
    // Do nothing
  }

  public void OverrideBehaviorScore(string behaviorName, float newScore) {
    // Do nothing
  }

  public void UpdateObservedObjectInfo(Anki.Cozmo.ExternalInterface.RobotObservedObject message) {
    // Won't be called
  }

  public void UpdateObservedFaceInfo(Anki.Cozmo.ExternalInterface.RobotObservedFace message) {
    // Won't be called
  }

  public void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams) {
    // we can update our display face
    CozmoFace.DisplayProceduralFace(faceAngle, faceCenter, faceScale, leftEyeParams, rightEyeParams);
  }

  public void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps) {
    LeftWheelSpeed = leftWheelSpeedMmps;
    RightWheelSpeed = rightWheelSpeedMmps;
  }

  private void QueueCallback(float delay, RobotCallback callback) {
    _Callbacks.Add(new CallbackWrapper(){ CallbackTime = Time.time + delay, Callback = callback });
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

  public void PlaceObjectRel(ObservedObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
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

  public void PlaceOnObject(ObservedObject target, float approachAngle, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    if (CarryingObject == null) {
      // Can't place object if carrying object is null
      if (callback != null) {
        callback(false);
        return;
      }
    }

    Rotation = Quaternion.Euler(0, 0, approachAngle);
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

  public void SendAnimation(string animName, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    // we can actually fake the callback by using CozmoFace
    float len = CozmoFace.PlayAnimation(animName);

    QueueCallback(len, callback);
  }

  public void SendAnimationGroup(string animGroupName, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    // lets just say half a second, since its random anyway
    QueueCallback(0.5f, callback);
  }

  public void SetIdleAnimation(string default_anim) {
    // Do nothing
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues, bool setUnspecifiedToDefault = false) {
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

  public void SetHeadAngle(float angleFactor = -0.8f, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW, bool useExactAngle = false, float accelRadSec = 2f, float maxSpeedFactor = 1f) {
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

  private float _RobotVolume = 0f;

  public void SetRobotVolume(float volume) {
    _RobotVolume = volume;
  }

  public float GetRobotVolume() {
    return _RobotVolume;
  }

  public void TrackToObject(ObservedObject observedObject, bool headOnly = true) {
    // TODO: Something?
    HeadTrackingObject = observedObject;
  }

  public ObservedObject GetCharger() {
    return null;
  }

  public void MountCharger(ObservedObject charger, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    
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

  public void FaceObject(ObservedObject observedObject, bool headTrackWhenDone = true, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    LookAtPosition(observedObject.WorldPosition);

    if (headTrackWhenDone) {
      TrackToObject(observedObject);
    }
    QueueCallback(1f, callback);
  }

  public void FacePose(Face face, float maxPanSpeed_radPerSec = 4.3f, float panAccel_radPerSec2 = 10f, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    LookAtPosition(face.WorldPosition);

    QueueCallback(1f, callback);
  }

  public void PickupObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0f, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    CarryingObject = selectedObject;
    LiftHeight = 1f;

    if (selectedObject != null) {
      Rotation = CarryingObject.Rotation.zRotation();
      CarryingObject.SetPosition(new Vector3(CarryingObject.WorldPosition.x, CarryingObject.WorldPosition.y, CozmoUtil.kCarriedObjectHeight));
      WorldPosition = (CarryingObject.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();
    }

    QueueCallback(5f, callback);
  }

  public void RollObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

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

  public void GotoObject(ObservedObject obj, float distance_mm, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    var delta = (WorldPosition - obj.WorldPosition).normalized * distance_mm;

    WorldPosition = (obj.WorldPosition + delta).xy0();

    QueueCallback(2f, callback);
  }

  public void AlignWithObject(ObservedObject obj, float distanceFromMarker_mm, RobotCallback callback = null, bool useApproachAngle = false, float approachAngleRad = 0f, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {

    Rotation = obj.Rotation.zRotation();
    WorldPosition = (obj.WorldPosition + Rotation * (Vector3.left * CozmoUtil.kOriginToLowLiftDDistMM)).xy0();

    QueueCallback(3f, callback);
  }

  public void SetLiftHeight(float heightFactor, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    LiftHeight = heightFactor;

    QueueCallback(0.2f, callback);
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

  public void TurnInPlace(float angle_rad,  float speed_rad_per_sec, float accel_rad_per_sec2, RobotCallback callback = null, Anki.Cozmo.QueueActionPosition queueActionPosition = Anki.Cozmo.QueueActionPosition.NOW) {
    Rotation *= Quaternion.Euler(0, 0, angle_rad);

    QueueCallback(0.5f, callback);
  }

  public void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false) {
    // Do something?
  }

  public void SetVisionMode(Anki.Cozmo.VisionMode mode, bool enable) {
    // Do nothing
  }

  public void ExecuteBehavior(Anki.Cozmo.BehaviorType type) {
    // Do nothing
  }

  public void SetBehaviorSystem(bool enable) {
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
    get ;
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

  public Anki.Cozmo.GameStatusFlag GameStatus {
    get;
    private set;
  }

  public float BatteryPercent {
    get { return 0.73f; }
  }

  private readonly List<ObservedObject> _VisibleObjects = new List<ObservedObject>();

  public List<ObservedObject> VisibleObjects {
    get {
      return _VisibleObjects;
    }
  }

  private readonly List<ObservedObject> _SeenObjects = new List<ObservedObject>();

  public List<ObservedObject> SeenObjects {
    get {
      return _SeenObjects;
    }
  }

  private readonly List<ObservedObject> _DirtyObjects = new List<ObservedObject>();

  public List<ObservedObject> DirtyObjects {
    get {
      return _DirtyObjects;
    }
  }

  private readonly Dictionary<int, LightCube> _LightCubes = new Dictionary<int, LightCube>();

  public Dictionary<int, LightCube> LightCubes {
    get {
      return _LightCubes;
    }
  }

  private readonly List<Face> _Faces = new List<Face>();

  public List<Face> Faces {
    get {
      return _Faces;
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

  public ObservedObject TargetLockedObject {
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

  private ObservedObject _CarryingObject;

  public ObservedObject CarryingObject {
    get { return _CarryingObject; }
    private set {
      _CarryingObject = value;
      if (OnCarryingObjectSet != null) {
        OnCarryingObjectSet(_CarryingObject);
      }
    }
  }

  private ObservedObject _HeadTrackingObject;

  public ObservedObject HeadTrackingObject {
    get { return _HeadTrackingObject; }
    private set {
      _HeadTrackingObject = value;
      if (OnHeadTrackingObjectSet != null) {
        OnHeadTrackingObjectSet(_HeadTrackingObject);
      }
    }
  }

  public bool IsBusy {
    get {
      return _Callbacks.Count > 0;
    }
  }

  #endregion

  #region IDisposable implementation

  public void Dispose() {
  }

  #endregion
  
}
