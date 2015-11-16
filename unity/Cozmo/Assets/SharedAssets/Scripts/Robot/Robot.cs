using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

/// <summary>
///   our unity side representation of cozmo's current state
///   also wraps most messages related solely to him
/// </summary>
public class Robot : IDisposable {
  public class Light {
    private uint _LastOnColor;
    public uint OnColor;
    private uint _LastOffColor;
    public uint OffColor;
    private uint _LastOnPeriodMs;
    public uint OnPeriodMs;
    private uint _LastOffPeriodMs;
    public uint OffPeriodMs;
    private uint _LastTransitionOnPeriodMs;
    public uint TransitionOnPeriodMs;
    private uint _LastTransitionOffPeriodMs;
    public uint TransitionOffPeriodMs;

    public void SetLastInfo() {
      _LastOnColor = OnColor;
      _LastOffColor = OffColor;
      _LastOnPeriodMs = OnPeriodMs;
      _LastOffPeriodMs = OffPeriodMs;
      _LastTransitionOnPeriodMs = TransitionOnPeriodMs;
      _LastTransitionOffPeriodMs = TransitionOffPeriodMs;
      _Initialized = true;
    }

    public bool Changed {
      get {
        return !_Initialized || _LastOnColor != OnColor || _LastOffColor != OffColor || _LastOnPeriodMs != OnPeriodMs || _LastOffPeriodMs != OffPeriodMs ||
        _LastTransitionOnPeriodMs != TransitionOnPeriodMs || _LastTransitionOffPeriodMs != TransitionOffPeriodMs;
      }
    }

    public virtual void ClearData() {
      OnColor = 0;
      OffColor = 0;
      OnPeriodMs = 1000;
      OffPeriodMs = 0;
      TransitionOnPeriodMs = 0;
      TransitionOffPeriodMs = 0;

      _LastOnColor = 0;
      _LastOffColor = 0;
      _LastOnPeriodMs = 1000;
      _LastOffPeriodMs = 0;
      _LastTransitionOnPeriodMs = 0;
      _LastTransitionOffPeriodMs = 0;

      MessageDelay = 0f;
    }

    public static float MessageDelay = 0f;

    public const uint FOREVER = 2147483647;
    private bool _Initialized = false;
  }

  public delegate void RobotCallback(bool success);

  public byte ID { get; private set; }

  // in radians
  public float HeadAngle { get; private set; }

  // in radians
  public float PoseAngle { get; private set; }

  // in mm/s
  public float LeftWheelSpeed { get; private set; }

  // in mm/s
  public float RightWheelSpeed { get; private set; }

  // in mm
  public float LiftHeight { get; private set; }

  public float LiftHeightFactor { get { return (LiftHeight - CozmoUtil.MIN_LIFT_HEIGHT_MM) / (CozmoUtil.MAX_LIFT_HEIGHT_MM - CozmoUtil.MIN_LIFT_HEIGHT_MM); } }

  public Vector3 WorldPosition { get; private set; }

  public Quaternion Rotation { get; private set; }

  public Vector3 Forward { get { return Rotation * Vector3.right; } }

  public Vector3 Right { get { return Rotation * -Vector3.up; } }

  public RobotStatusFlag RobotStatus { get; private set; }

  public GameStatusFlag GameStatus { get; private set; }

  public float BatteryPercent { get; private set; }

  public List<ObservedObject> VisibleObjects { get; private set; }

  public List<ObservedObject> SeenObjects { get; private set; }

  public List<ObservedObject> DirtyObjects { get; private set; }

  public Dictionary<int, LightCube> LightCubes { get; private set; }

  public List<Face> Faces { get; private set; }

  public uint[] ProgressionStats { get; private set; }

  public float[] EmotionValues { get; private set; }

  public Light[] Lights { get; private set; }

  private bool _LightsChanged {
    get {
      for (int i = 0; i < Lights.Length; ++i) {
        if (Lights[i].Changed)
          return true;
      }

      return false;
    }
  }

  private ObservedObject _targetLockedObject = null;

  public ObservedObject targetLockedObject {
    get { return _targetLockedObject; }
    set {
      _targetLockedObject = value;
    }
  }

  private int _CarryingObjectID;

  public int HeadTrackingObjectID { get; private set; }

  private int _LastHeadTrackingObjectID;

  private float _HeadAngleRequested;
  private float _LastHeadAngleRequestTime;
  private float _LiftHeightRequested;
  private float _LastLiftHeightRequestTime;

  private U2G.DriveWheels DriveWheelsMessage;
  private U2G.PlaceObjectOnGroundHere PlaceObjectOnGroundHereMessage;
  private U2G.CancelAction CancelActionMessage;
  private U2G.SetHeadAngle SetHeadAngleMessage;
  private U2G.TrackToObject TrackToObjectMessage;
  private U2G.FaceObject FaceObjectMessage;
  private U2G.FacePose FacePoseMessage;
  private U2G.PickupObject PickupObjectMessage;
  private U2G.RollObject RollObjectMessage;
  private U2G.PlaceObjectOnGround PlaceObjectOnGroundMessage;
  private U2G.GotoPose GotoPoseMessage;
  private U2G.GotoObject GotoObjectMessage;
  private U2G.SetLiftHeight SetLiftHeightMessage;
  private U2G.SetRobotCarryingObject SetRobotCarryingObjectMessage;
  private U2G.ClearAllBlocks ClearAllBlocksMessage;
  private U2G.ClearAllObjects ClearAllObjectsMessage;
  private U2G.VisionWhileMoving VisionWhileMovingMessage;
  private U2G.TurnInPlace TurnInPlaceMessage;
  private U2G.TraverseObject TraverseObjectMessage;
  private U2G.SetBackpackLEDs SetBackpackLEDsMessage;
  private U2G.EnableVisionMode EnableVisionModeMessage;
  private U2G.ExecuteBehavior ExecuteBehaviorMessage;
  private U2G.SetBehaviorSystemEnabled SetBehaviorSystemEnabledMessage;
  private U2G.ActivateBehaviorChooser ActivateBehaviorChooserMessage;
  private U2G.PlaceRelObject PlaceRelObjectMessage;
  private U2G.PlayAnimation PlayAnimationMessage;
  private U2G.PlayAnimation[] PlayAnimationMessages;
  private U2G.SetIdleAnimation SetIdleAnimationMessage;
  private U2G.SetLiveIdleAnimationParameters SetLiveIdleAnimationParametersMessage;
  private U2G.SetRobotVolume SetRobotVolumeMessage;
  private U2G.AlignWithObject AlignWithObjectMessage;
  private U2G.ProgressionMessage ProgressionStatMessage;
  private U2G.MoodMessage MoodStatMessage;
 
  private PathMotionProfile PathMotionProfileDefault;

  private ObservedObject _CarryingObject;

  public ObservedObject CarryingObject {
    get {
      if (_CarryingObject != _CarryingObjectID)
        _CarryingObject = SeenObjects.Find(x => x == _CarryingObjectID);

      return _CarryingObject;
    }
  }

  private ObservedObject _HeadTrackingObject;

  public ObservedObject HeadTrackingObject {
    get {
      if (_HeadTrackingObject != HeadTrackingObjectID)
        _HeadTrackingObject = SeenObjects.Find(x => x == HeadTrackingObjectID);
      
      return _HeadTrackingObject;
    }
  }

  private List<KeyValuePair<RobotActionType, RobotCallback>> _RobotCallbacks = new List<KeyValuePair<RobotActionType, RobotCallback>>();

  [System.NonSerialized] public float LocalBusyTimer = 0f;
  [System.NonSerialized] public bool LocalBusyOverride = false;

  public bool IsBusy {
    get {
      return LocalBusyOverride
      || LocalBusyTimer > 0f
      || Status(RobotStatusFlag.IS_PATHING)
      || (Status(RobotStatusFlag.IS_ANIMATING) && !Status(RobotStatusFlag.IS_ANIMATING_IDLE))
      || Status(RobotStatusFlag.IS_PICKED_UP);
    }

    set {
      LocalBusyOverride = value;

      if (value) {
        DriveWheels(0, 0); 
      }
    }
  }

  public bool Status(RobotStatusFlag s) {
    return (RobotStatus & s) == s;
  }

  public bool IsLocalized() {
    return (GameStatus & GameStatusFlag.IsLocalized) == GameStatusFlag.IsLocalized;
  }


  public Robot(byte robotID) : base() {
    ID = robotID;
    const int initialSize = 8;
    SeenObjects = new List<ObservedObject>(initialSize);
    VisibleObjects = new List<ObservedObject>(initialSize);
    DirtyObjects = new List<ObservedObject>(initialSize);
    LightCubes = new Dictionary<int, LightCube>();
    Faces = new List< global::Face>();

    DriveWheelsMessage = new U2G.DriveWheels();
    PlaceObjectOnGroundHereMessage = new U2G.PlaceObjectOnGroundHere();
    CancelActionMessage = new U2G.CancelAction();
    SetHeadAngleMessage = new U2G.SetHeadAngle();
    TrackToObjectMessage = new U2G.TrackToObject();
    FaceObjectMessage = new U2G.FaceObject();
    FacePoseMessage = new G2U.FacePose();
    PickupObjectMessage = new U2G.PickupObject();
    RollObjectMessage = new U2G.RollObject();
    PlaceObjectOnGroundMessage = new U2G.PlaceObjectOnGround();
    GotoPoseMessage = new U2G.GotoPose();
    GotoObjectMessage = new U2G.GotoObject();
    SetLiftHeightMessage = new U2G.SetLiftHeight();
    SetRobotCarryingObjectMessage = new U2G.SetRobotCarryingObject();
    ClearAllBlocksMessage = new U2G.ClearAllBlocks();
    ClearAllObjectsMessage = new U2G.ClearAllObjects();
    VisionWhileMovingMessage = new U2G.VisionWhileMoving();
    TurnInPlaceMessage = new U2G.TurnInPlace();
    TraverseObjectMessage = new U2G.TraverseObject();
    SetBackpackLEDsMessage = new U2G.SetBackpackLEDs();
    EnableVisionModeMessage = new U2G.EnableVisionMode();
    ExecuteBehaviorMessage = new U2G.ExecuteBehavior();
    SetBehaviorSystemEnabledMessage = new U2G.SetBehaviorSystemEnabled();
    ActivateBehaviorChooserMessage = new U2G.ActivateBehaviorChooser();
    PlaceRelObjectMessage = new U2G.PlaceRelObject();
    PlayAnimationMessage = new U2G.PlayAnimation();
    PlayAnimationMessages = new U2G.PlayAnimation[2];
    PlayAnimationMessages[0] = new U2G.PlayAnimation();
    PlayAnimationMessages[1] = new U2G.PlayAnimation();
    SetRobotVolumeMessage = new U2G.SetRobotVolume();
    AlignWithObjectMessage = new U2G.AlignWithObject();
    ProgressionStatMessage = new U2G.ProgressionMessage();
    MoodStatMessage = new U2G.MoodMessage();

    // These defaults should eventually be in clad
    PathMotionProfileDefault = new PathMotionProfile();
    PathMotionProfileDefault.speed_mmps = 60.0f;
    PathMotionProfileDefault.accel_mmps2 = 200.0f;
    PathMotionProfileDefault.decel_mmps2 = 500.0f;
    PathMotionProfileDefault.pointTurnSpeed_rad_per_sec = 2.0f;
    PathMotionProfileDefault.pointTurnAccel_rad_per_sec2 = 100.0f;
    PathMotionProfileDefault.pointTurnDecel_rad_per_sec2 = 500.0f;

    SetIdleAnimationMessage = new U2G.SetIdleAnimation();
    SetLiveIdleAnimationParametersMessage = new U2G.SetLiveIdleAnimationParameters();

    Lights = new Light[SetBackpackLEDsMessage.onColor.Length];
    ProgressionStats = new uint[(int)Anki.Cozmo.ProgressionStatType.Count];
    EmotionValues = new float[(int)Anki.Cozmo.EmotionType.Count];

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i] = new Light();
    }

    ClearData(true);

    RobotEngineManager.Instance.DisconnectedFromClient += Reset;

    RobotEngineManager.Instance.SuccessOrFailure += RobotEngineMessages;
  }

  public void Dispose() {
    RobotEngineManager.Instance.DisconnectedFromClient -= Reset;
    RobotEngineManager.Instance.SuccessOrFailure -= RobotEngineMessages;
  }

  public void CooldownTimers(float delta) {
    if (LocalBusyTimer > 0f) {
      LocalBusyTimer -= delta;
    }
  }

  private void RobotEngineMessages(bool success, RobotActionType messageType) {
    DAS.Info("Robot.ActionCallback", "Type = " + messageType + " success = " + success);
    for (int i = 0; i < _RobotCallbacks.Count; ++i) {
      if (messageType == _RobotCallbacks[i].Key) {
        _RobotCallbacks[i].Value(success);
        _RobotCallbacks.RemoveAt(i);
        i--;
      }
    }
  }

  private int SortByDistance(ObservedObject a, ObservedObject b) {
    float d1 = ((Vector2)a.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
    float d2 = ((Vector2)b.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
    return d1.CompareTo(d2);
  }

  private void Reset(DisconnectionReason reason = DisconnectionReason.None) {
    ClearData();
  }

  public void ClearData(bool initializing = false) {
    if (!initializing) {
      TurnOffAllLights(true);
      DAS.Debug("Robot", "Robot data cleared");
    }

    SeenObjects.Clear();
    VisibleObjects.Clear();
    LightCubes.Clear();
    RobotStatus = RobotStatusFlag.NoneRobotStatusFlag;
    GameStatus = GameStatusFlag.Nothing;
    WorldPosition = Vector3.zero;
    Rotation = Quaternion.identity;
    _CarryingObjectID = -1;
    HeadTrackingObjectID = -1;
    _LastHeadTrackingObjectID = -1;
    targetLockedObject = null;
    _CarryingObject = null;
    _HeadTrackingObject = null;
    _HeadAngleRequested = float.MaxValue;
    _LiftHeightRequested = float.MaxValue;
    HeadAngle = float.MaxValue;
    PoseAngle = float.MaxValue;
    LeftWheelSpeed = float.MaxValue;
    RightWheelSpeed = float.MaxValue;
    LiftHeight = float.MaxValue;
    BatteryPercent = float.MaxValue;
    WorldPosition = Vector3.zero;
    Rotation = Quaternion.identity;
    LocalBusyTimer = 0f;

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].ClearData();
    }

    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; ++i) {
      ProgressionStats[i] = 0;
    }
  }

  public void ClearVisibleObjects() {
    for (int i = 0; i < VisibleObjects.Count; ++i) {
      if (VisibleObjects[i].TimeLastSeen + ObservedObject.RemoveDelay < Time.time) {
        VisibleObjects.RemoveAt(i--);
      }
    }
  }

  public void UpdateInfo(G2U.RobotState message) {
    HeadAngle = message.headAngle_rad;
    PoseAngle = message.poseAngle_rad;
    LeftWheelSpeed = message.leftWheelSpeed_mmps;
    RightWheelSpeed = message.rightWheelSpeed_mmps;
    LiftHeight = message.liftHeight_mm;
    RobotStatus = (RobotStatusFlag)message.status;
    GameStatus = (GameStatusFlag)message.gameStatus;
    BatteryPercent = (message.batteryVoltage / CozmoUtil.MAX_VOLTAGE);
    _CarryingObjectID = message.carryingObjectID;
    HeadTrackingObjectID = message.headTrackingObjectID;

    if (HeadTrackingObjectID == _LastHeadTrackingObjectID)
      _LastHeadTrackingObjectID = -1;

    WorldPosition = new Vector3(message.pose_x, message.pose_y, message.pose_z);
    Rotation = new Quaternion(message.pose_qx, message.pose_qy, message.pose_qz, message.pose_qw);

  }

  public void UpdateLightMessages(bool now = false) {
    if (RobotEngineManager.Instance == null || !RobotEngineManager.Instance.IsConnected)
      return;

    if (Time.time > LightCube.Light.MessageDelay || now) {
      var enumerator = LightCubes.GetEnumerator();

      while (enumerator.MoveNext()) {
        LightCube lightCube = enumerator.Current.Value;

        if (lightCube.LightsChanged)
          lightCube.SetAllLEDs();
      }
    }

    if (Time.time > Light.MessageDelay || now) {
      if (_LightsChanged)
        SetAllBackpackLEDs();
    }
  }

  // Object moved, so remove it from the seen list
  // and add it into the dirty list.
  public void UpdateDirtyList(ObservedObject dirty) {
    SeenObjects.Remove(dirty);

    if (!DirtyObjects.Contains(dirty)) {
      DirtyObjects.Add(dirty);
    }
  }

  #region Progression and Mood Stats

  public void AddToProgressionStat(Anki.Cozmo.ProgressionStatType index, uint deltaValue) {
    ProgressionStatMessage.robotID = ID;
    ProgressionStatMessage.ProgressionMessageUnion.AddToProgressionStat.statType = index;
    ProgressionStatMessage.ProgressionMessageUnion.AddToProgressionStat.deltaVal = deltaValue;

    RobotEngineManager.Instance.Message.ProgressionMessage = ProgressionStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void AddToEmotion(Anki.Cozmo.EmotionType type, float deltaValue) {
    MoodStatMessage.robotID = ID;
    MoodStatMessage.MoodMessageUnion.AddToEmotion.emotionType = type;
    MoodStatMessage.MoodMessageUnion.AddToEmotion.deltaVal = deltaValue;

    RobotEngineManager.Instance.Message.MoodMessage = MoodStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  // Only debug panels should be using set.
  // You should not be calling this from a minigame/challenge.
  public void SetEmotion(Anki.Cozmo.EmotionType type, float value) {
    MoodStatMessage.robotID = ID;
    MoodStatMessage.MoodMessageUnion.SetEmotion.emotionType = type;
    MoodStatMessage.MoodMessageUnion.SetEmotion.newVal = value;

    RobotEngineManager.Instance.Message.MoodMessage = MoodStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  // Only debug panels should be using set.
  // You should not be calling this from a minigame/challenge.
  public void SetProgressionStat(Anki.Cozmo.ProgressionStatType type, uint value) {
    ProgressionStatMessage.robotID = ID;
    ProgressionStatMessage.ProgressionMessageUnion.SetProgressionStat.statType = type;
    ProgressionStatMessage.ProgressionMessageUnion.SetProgressionStat.newVal = value;

    RobotEngineManager.Instance.Message.ProgressionMessage = ProgressionStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  // This function should only be called by RobotEngineManager to update the internal Progression Stat values.
  public void UpdateProgressionStatFromEngineRobotManager(Anki.Cozmo.ProgressionStatType index, uint value) {
    ProgressionStats[(int)index] = value;
  }

  // This function should only be called by RobotEngineManager to update the internal emotion Stat values.
  public void UpdateEmotionFromEngineRobotManager(Anki.Cozmo.EmotionType index, float value) {
    EmotionValues[(int)index] = value;
  }

  #endregion

  public void UpdateObservedObjectInfo(G2U.RobotObservedObject message) {
    if (message.objectFamily == Anki.Cozmo.ObjectFamily.Mat) {
      DAS.Warn("Robot", "UpdateObservedObjectInfo received message about the Mat!");
      return;
    }

    if (message.objectFamily == Anki.Cozmo.ObjectFamily.LightCube) {
      AddLightCube(LightCubes.ContainsKey(message.objectID) ? LightCubes[message.objectID] : null, message);
    }
    else {
      ObservedObject knownObject = SeenObjects.Find(x => x == message.objectID);

      AddObservedObject(knownObject, message);
    }
  }

  private void AddLightCube(LightCube lightCube, G2U.RobotObservedObject message) {
    if (lightCube == null) {
      lightCube = new LightCube(message.objectID, message.objectFamily, message.objectType);

      LightCubes.Add(lightCube, lightCube);
      SeenObjects.Add(lightCube);
    }

    AddObservedObject(lightCube, message);
  }

  private void AddObservedObject(ObservedObject knownObject, G2U.RobotObservedObject message) {
    if (knownObject == null) {
      knownObject = new ObservedObject(message.objectID, message.objectFamily, message.objectType);
      SeenObjects.Add(knownObject);
    }
    else {
      DirtyObjects.Remove(knownObject);
    }

    knownObject.UpdateInfo(message);
    
    if (SeenObjects.Find(x => x == message.objectID) == null) {
      SeenObjects.Add(knownObject);
    }
    
    if (knownObject.MarkersVisible && VisibleObjects.Find(x => x == message.objectID) == null) {
      VisibleObjects.Add(knownObject);
    }
  }

  public void UpdateObservedFaceInfo(G2U.RobotObservedFace message) {
    //DAS.Debug ("Robot", "saw a face at " + message.faceID);
    Face face = Faces.Find(x => x.ID == message.faceID);
    AddObservedFace(face != null ? face : null, message);
  }

  private void AddObservedFace(Face faceObject, G2U.RobotObservedFace message) {
    if (faceObject == null) {
      faceObject = new Face(message);
      Faces.Add(faceObject);
    }
    else {
      faceObject.UpdateInfo(message);
    }
  }

  public void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps) {
    DriveWheelsMessage.lwheel_speed_mmps = leftWheelSpeedMmps;
    DriveWheelsMessage.rwheel_speed_mmps = rightWheelSpeedMmps;

    RobotEngineManager.Instance.Message.DriveWheels = DriveWheelsMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void PlaceObjectOnGroundHere(RobotCallback callback = null) {
    DAS.Debug("Robot", "Place Object " + CarryingObject + " On Ground Here");

    RobotEngineManager.Instance.Message.PlaceObjectOnGroundHere = PlaceObjectOnGroundHereMessage;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_LOW, callback));
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_HIGH, callback));
    }
  }

  public void PlaceObjectRel(ObservedObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null) {
    DAS.Debug("Robot", "PlaceObjectRel" + target.ID);

    PlaceRelObjectMessage.approachAngle_rad = approachAngle;
    PlaceRelObjectMessage.placementOffsetX_mm = offsetFromMarker;
    PlaceRelObjectMessage.objectID = target.ID;
    PlaceRelObjectMessage.useApproachAngle = true;
    PlaceRelObjectMessage.usePreDockPose = true;
    PlaceRelObjectMessage.useManualSpeed = false;
    PlaceRelObjectMessage.motionProf = PathMotionProfileDefault;

    RobotEngineManager.Instance.Message.PlaceRelObject = PlaceRelObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_LOW, callback));
    }
  }

  public void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN) {
    CancelActionMessage.robotID = ID;
    CancelActionMessage.actionType = actionType;

    DAS.Debug("Robot", "CancelAction actionType(" + actionType + ")");

    RobotEngineManager.Instance.Message.CancelAction = CancelActionMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void SendAnimation(string animName, RobotCallback callback = null) {

    DAS.Debug("CozmoEmotionManager", "Sending " + animName + " with " + 1 + " loop");
    PlayAnimationMessage.animationName = animName;
    PlayAnimationMessage.numLoops = 1;
    PlayAnimationMessage.robotID = this.ID;

    RobotEngineManager.Instance.Message.PlayAnimation = PlayAnimationMessage;
    RobotEngineManager.Instance.SendMessage();

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLAY_ANIMATION, callback));
    }
  }

  public void SetIdleAnimation(string default_anim) {
    SetIdleAnimationMessage.animationName = default_anim;
    SetIdleAnimationMessage.robotID = this.ID;
    RobotEngineManager.Instance.Message.SetIdleAnimation = SetIdleAnimationMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues) {

    SetLiveIdleAnimationParametersMessage.paramNames = paramNames;
    SetLiveIdleAnimationParametersMessage.paramValues = paramValues;
    SetLiveIdleAnimationParametersMessage.robotID = 1;

    RobotEngineManager.Instance.Message.SetLiveIdleAnimationParameters = SetLiveIdleAnimationParametersMessage;
    RobotEngineManager.Instance.SendMessage();

  }

  public float GetHeadAngleFactor() {

    float angle = IsHeadAngleRequestUnderway() ? _HeadAngleRequested : HeadAngle;

    if (angle >= 0f) {
      angle = Mathf.Lerp(0f, 1f, angle / (CozmoUtil.MAX_HEAD_ANGLE * Mathf.Deg2Rad));
    }
    else {
      angle = Mathf.Lerp(0f, -1f, angle / (CozmoUtil.MIN_HEAD_ANGLE * Mathf.Deg2Rad));
    }

    return angle;
  }


  public bool IsHeadAngleRequestUnderway() {
    return Time.time < _LastHeadAngleRequestTime + CozmoUtil.HEAD_ANGLE_REQUEST_TIME;
  }

  /// <summary>
  /// Sets the head angle.
  /// </summary>
  /// <param name="angleFactor">Angle factor.</param> usually from -1 (MIN_HEAD_ANGLE) to 1 (MAX_HEAD_ANGLE)
  /// <param name="useExactAngle">If set to <c>true</c> angleFactor is treated as an exact angle in radians.</param>
  public void SetHeadAngle(float angleFactor = -0.8f, 
                           Robot.RobotCallback onComplete = null,
                           bool useExactAngle = false, 
                           float accelRadSec = 2f, 
                           float maxSpeedFactor = 1f) {

    float radians = angleFactor;

    if (!useExactAngle) {
      if (angleFactor >= 0f) {
        radians = Mathf.Lerp(0f, CozmoUtil.MAX_HEAD_ANGLE * Mathf.Deg2Rad, angleFactor);
      }
      else {
        radians = Mathf.Lerp(0f, CozmoUtil.MIN_HEAD_ANGLE * Mathf.Deg2Rad, -angleFactor);
      }
    }

    if (IsHeadAngleRequestUnderway() && Mathf.Abs(_HeadAngleRequested - radians) < 0.001f)
      return;
    if (HeadTrackingObject == -1 && Mathf.Abs(radians - HeadAngle) < 0.001f)
      return;

    _HeadAngleRequested = radians;
    _LastHeadAngleRequestTime = Time.time;

    SetHeadAngleMessage.angle_rad = radians;

    SetHeadAngleMessage.accel_rad_per_sec2 = accelRadSec;
    SetHeadAngleMessage.max_speed_rad_per_sec = maxSpeedFactor * CozmoUtil.MAX_SPEED_RAD_PER_SEC;

    RobotEngineManager.Instance.Message.SetHeadAngle = SetHeadAngleMessage;
    RobotEngineManager.Instance.SendMessage();

    if (onComplete != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.MOVE_HEAD_TO_ANGLE, onComplete));
    }
  }

  public void SetRobotVolume(float volume) {
    SetRobotVolumeMessage.volume = volume;
    DAS.Debug("RobotEngineManager", "Set Robot Volume " + SetRobotVolumeMessage.volume);

    RobotEngineManager.Instance.Message.SetRobotVolume = SetRobotVolumeMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public float GetRobotVolume() {
    return SetRobotVolumeMessage.volume;
  }

  public void TrackToObject(ObservedObject observedObject, bool headOnly = true) {
    if (HeadTrackingObjectID == observedObject) {
      _LastHeadTrackingObjectID = -1;
      return;
    }
    else if (_LastHeadTrackingObjectID == observedObject) {
      return;
    }

    _LastHeadTrackingObjectID = observedObject;

    if (observedObject != null) {
      TrackToObjectMessage.objectID = (uint)observedObject;
    }
    else {
      TrackToObjectMessage.objectID = uint.MaxValue; //cancels tracking
    }

    TrackToObjectMessage.robotID = ID;
    TrackToObjectMessage.headOnly = headOnly;

    DAS.Debug("Robot", "Track Head To Object " + TrackToObjectMessage.objectID);

    RobotEngineManager.Instance.Message.TrackToObject = TrackToObjectMessage;
    RobotEngineManager.Instance.SendMessage();

  }

  public void FaceObject(ObservedObject observedObject, bool headTrackWhenDone = true) {
    FaceObjectMessage.objectID = observedObject;
    FaceObjectMessage.robotID = ID;
    FaceObjectMessage.maxTurnAngle = float.MaxValue;
    FaceObjectMessage.turnAngleTol = Mathf.Deg2Rad; //one degree seems to work?
    FaceObjectMessage.headTrackWhenDone = System.Convert.ToByte(headTrackWhenDone);
    
    DAS.Debug("Robot", "Face Object " + FaceObjectMessage.objectID);

    RobotEngineManager.Instance.Message.FaceObject = FaceObjectMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void FacePose(Face face) {
    FacePoseMessage.maxTurnAngle = float.MaxValue;
    FacePoseMessage.robotID = ID;
    FacePoseMessage.turnAngleTol = Mathf.Deg2Rad; //one degree seems to work?
    FacePoseMessage.world_x = face.WorldPosition.x;
    FacePoseMessage.world_y = face.WorldPosition.y;
    FacePoseMessage.world_z = face.WorldPosition.z;

    RobotEngineManager.Instance.Message.FacePose = FacePoseMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void PickupObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, RobotCallback callback = null) {
    PickupObjectMessage.objectID = selectedObject;
    PickupObjectMessage.usePreDockPose = usePreDockPose;
    PickupObjectMessage.useManualSpeed = useManualSpeed;
    PickupObjectMessage.useApproachAngle = useApproachAngle;
    PickupObjectMessage.approachAngle_rad = approachAngleRad;
    PickupObjectMessage.motionProf = PathMotionProfileDefault;
    
    DAS.Debug("Robot", "Pick And Place Object " + PickupObjectMessage.objectID + " usePreDockPose " + PickupObjectMessage.usePreDockPose + " useManualSpeed " + PickupObjectMessage.useManualSpeed);

    RobotEngineManager.Instance.Message.PickupObject = PickupObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PICKUP_OBJECT_LOW, callback));
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PICK_AND_PLACE_INCOMPLETE, callback));
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PICKUP_OBJECT_HIGH, callback));
    }
  }

  public void RollObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, RobotCallback callback = null) {
    RollObjectMessage.objectID = selectedObject;
    RollObjectMessage.usePreDockPose = usePreDockPose;
    RollObjectMessage.useManualSpeed = useManualSpeed;
    RollObjectMessage.motionProf = PathMotionProfileDefault;

    DAS.Debug("Robot", "Roll Object " + RollObjectMessage.objectID + " usePreDockPose " + RollObjectMessage.usePreDockPose + " useManualSpeed " + RollObjectMessage.useManualSpeed);
    
    RobotEngineManager.Instance.Message.RollObject = RollObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.ROLL_OBJECT_LOW, callback));
    }
  }

  public void PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null) {
    PlaceObjectOnGroundMessage.x_mm = position.x;
    PlaceObjectOnGroundMessage.y_mm = position.y;
    PlaceObjectOnGroundMessage.qx = rotation.x;
    PlaceObjectOnGroundMessage.qy = rotation.y;
    PlaceObjectOnGroundMessage.qz = rotation.z;
    PlaceObjectOnGroundMessage.qw = rotation.w;
    PlaceObjectOnGroundMessage.level = System.Convert.ToByte(level);
    PlaceObjectOnGroundMessage.useManualSpeed = useManualSpeed;
    PlaceObjectOnGroundMessage.motionProf = PathMotionProfileDefault;
    
    DAS.Debug("Robot", "Drop Object At Pose " + position + " useManualSpeed " + useManualSpeed);
    
    RobotEngineManager.Instance.Message.PlaceObjectOnGround = PlaceObjectOnGroundMessage;
    RobotEngineManager.Instance.SendMessage();
    
    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_LOW, callback));
    }

  }

  public void GotoPose(float x_mm, float y_mm, float rad, RobotCallback callback = null, bool level = false, bool useManualSpeed = false) {
    GotoPoseMessage.level = System.Convert.ToByte(level);
    GotoPoseMessage.useManualSpeed = useManualSpeed;
    GotoPoseMessage.x_mm = x_mm;
    GotoPoseMessage.y_mm = y_mm;
    GotoPoseMessage.rad = rad;
    GotoPoseMessage.motionProf = PathMotionProfileDefault;

    DAS.Debug("Robot", "Go to Pose: x: " + GotoPoseMessage.x_mm + " y: " + GotoPoseMessage.y_mm + " useManualSpeed: " + GotoPoseMessage.useManualSpeed + " level: " + GotoPoseMessage.level);

    RobotEngineManager.Instance.Message.GotoPose = GotoPoseMessage;
    RobotEngineManager.Instance.SendMessage();
    
    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.DRIVE_TO_POSE, callback));
    }
  }

  public void GotoObject(ObservedObject obj, float distance_mm, RobotCallback callback = null) {
    GotoObjectMessage.objectID = obj;
    GotoObjectMessage.distanceFromObjectOrigin_mm = distance_mm;
    GotoObjectMessage.useManualSpeed = false;
    GotoObjectMessage.motionProf = PathMotionProfileDefault;

    RobotEngineManager.Instance.Message.GotoObject = GotoObjectMessage;

    RobotEngineManager.Instance.SendMessage();
    
    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.DRIVE_TO_OBJECT, callback));
    }
  }

  public void AlignWithObject(ObservedObject obj, float distanceFromMarker_mm, RobotCallback callback = null) {
    AlignWithObjectMessage.objectID = obj;
    AlignWithObjectMessage.distanceFromMarker_mm = distanceFromMarker_mm;
    AlignWithObjectMessage.useManualSpeed = false;
    AlignWithObjectMessage.useApproachAngle = false;
    AlignWithObjectMessage.motionProf = PathMotionProfileDefault;

    RobotEngineManager.Instance.Message.AlignWithObject = AlignWithObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.ALIGN_WITH_OBJECT, callback));
    }
  }

  // Height factor should be between 0.0f and 1.0f
  // 0.0f being lowest and 1.0f being highest.
  public void SetLiftHeight(float heightFactor, RobotCallback callback = null) {
    DAS.Debug("Robot", "SetLiftHeight: " + heightFactor);
    if ((Time.time < _LastLiftHeightRequestTime + CozmoUtil.LIFT_REQUEST_TIME && heightFactor == _LiftHeightRequested) || LiftHeightFactor == heightFactor)
      return;

    _LiftHeightRequested = heightFactor;
    _LastLiftHeightRequestTime = Time.time;

    SetLiftHeightMessage.accel_rad_per_sec2 = 5f;
    SetLiftHeightMessage.max_speed_rad_per_sec = 10f;
    SetLiftHeightMessage.height_mm = (heightFactor * (CozmoUtil.MAX_LIFT_HEIGHT_MM - CozmoUtil.MIN_LIFT_HEIGHT_MM)) + CozmoUtil.MIN_LIFT_HEIGHT_MM;

    RobotEngineManager.Instance.Message.SetLiftHeight = SetLiftHeightMessage;
    RobotEngineManager.Instance.SendMessage();

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.MOVE_LIFT_TO_HEIGHT, callback));
    }
  }

  public void SetRobotCarryingObject(int objectID = -1) {
    DAS.Debug("Robot", "Set Robot Carrying Object: " + objectID);
    
    SetRobotCarryingObjectMessage.robotID = ID;
    SetRobotCarryingObjectMessage.objectID = objectID;

    RobotEngineManager.Instance.Message.SetRobotCarryingObject = SetRobotCarryingObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    targetLockedObject = null;
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  public void ClearAllBlocks() {
    DAS.Debug("Robot", "Clear All Blocks");
    ClearAllBlocksMessage.robotID = ID;
    RobotEngineManager.Instance.Message.ClearAllBlocks = ClearAllBlocksMessage;
    RobotEngineManager.Instance.SendMessage();
    Reset();
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  public void ClearAllObjects() {
    ClearAllObjectsMessage.robotID = ID;
    RobotEngineManager.Instance.Message.ClearAllObjects = ClearAllObjectsMessage;
    RobotEngineManager.Instance.SendMessage();
    Reset();
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  public void VisionWhileMoving(bool enable) {
    VisionWhileMovingMessage.enable = System.Convert.ToByte(enable);

    RobotEngineManager.Instance.Message.VisionWhileMoving = VisionWhileMovingMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void TurnInPlace(float angle_rad, RobotCallback callback = null) {
    TurnInPlaceMessage.robotID = ID;
    TurnInPlaceMessage.angle_rad = angle_rad;

    RobotEngineManager.Instance.Message.TurnInPlace = TurnInPlaceMessage;
    RobotEngineManager.Instance.SendMessage();

    if (callback != null) {
      _RobotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.TURN_IN_PLACE, callback));
    }
  }

  public void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false) {
    DAS.Debug("Robot", "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose);

    TraverseObjectMessage.useManualSpeed = useManualSpeed;
    TraverseObjectMessage.usePreDockPose = usePreDockPose;

    RobotEngineManager.Instance.Message.TraverseObject = TraverseObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
  }

  private void SetLastLEDs() {
    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].SetLastInfo();
    }
  }

  public void SetBackpackLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                              uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {
    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].OnColor = onColor;
      Lights[i].OffColor = offColor;
      Lights[i].OnPeriodMs = onPeriod_ms;
      Lights[i].OffPeriodMs = offPeriod_ms;
      Lights[i].TransitionOnPeriodMs = transitionOnPeriod_ms;
      Lights[i].TransitionOffPeriodMs = transitionOffPeriod_ms;
    }
  }

  // should only be called from update loop
  private void SetAllBackpackLEDs() {

    SetBackpackLEDsMessage.robotID = ID;

    for (int i = 0; i < Lights.Length; ++i) {
      SetBackpackLEDsMessage.onColor[i] = Lights[i].OnColor;
      SetBackpackLEDsMessage.offColor[i] = Lights[i].OffColor;
      SetBackpackLEDsMessage.onPeriod_ms[i] = Lights[i].OnPeriodMs;
      SetBackpackLEDsMessage.offPeriod_ms[i] = Lights[i].OffPeriodMs;
      SetBackpackLEDsMessage.transitionOnPeriod_ms[i] = Lights[i].TransitionOnPeriodMs;
      SetBackpackLEDsMessage.transitionOffPeriod_ms[i] = Lights[i].TransitionOffPeriodMs;
    }
    
    RobotEngineManager.Instance.Message.SetBackpackLEDs = SetBackpackLEDsMessage;
    RobotEngineManager.Instance.SendMessage();

    SetLastLEDs();
  }

  public void SetVisionMode(VisionMode mode, bool enable) {
    EnableVisionModeMessage.mode = mode;
    EnableVisionModeMessage.enable = enable;

    RobotEngineManager.Instance.Message.EnableVisionMode = EnableVisionModeMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void TurnOffAllLights(bool now = false) {
    var enumerator = LightCubes.GetEnumerator();
    
    while (enumerator.MoveNext()) {
      LightCube lightCube = enumerator.Current.Value;
      
      lightCube.SetLEDs();
    }

    SetBackpackLEDs();

    if (now)
      UpdateLightMessages(now);
  }

  public void ExecuteBehavior(BehaviorType type) {
    ExecuteBehaviorMessage.behaviorType = type;
    
    DAS.Debug("Robot", "Execute Behavior " + ExecuteBehaviorMessage.behaviorType);

    RobotEngineManager.Instance.Message.ExecuteBehavior = ExecuteBehaviorMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetBehaviorSystem(bool enable) {
    SetBehaviorSystemEnabledMessage.enabled = enable;

    RobotEngineManager.Instance.Message.SetBehaviorSystemEnabled = SetBehaviorSystemEnabledMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void ActivateBehaviorChooser(BehaviorChooserType behaviorChooserType) {
    ActivateBehaviorChooserMessage.behaviorChooserType = behaviorChooserType;

    RobotEngineManager.Instance.Message.ActivateBehaviorChooser = ActivateBehaviorChooserMessage;
    RobotEngineManager.Instance.SendMessage();
  }
}
