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
    private uint lastOnColor;
    public uint onColor;
    private uint lastOffColor;
    public uint offColor;
    private uint lastOnPeriod_ms;
    public uint onPeriod_ms;
    private uint lastOffPeriod_ms;
    public uint offPeriod_ms;
    private uint lastTransitionOnPeriod_ms;
    public uint transitionOnPeriod_ms;
    private uint lastTransitionOffPeriod_ms;
    public uint transitionOffPeriod_ms;

    public void SetLastInfo() {
      lastOnColor = onColor;
      lastOffColor = offColor;
      lastOnPeriod_ms = onPeriod_ms;
      lastOffPeriod_ms = offPeriod_ms;
      lastTransitionOnPeriod_ms = transitionOnPeriod_ms;
      lastTransitionOffPeriod_ms = transitionOffPeriod_ms;
      initialized = true;
    }

    public bool changed {
      get {
        return !initialized || lastOnColor != onColor || lastOffColor != offColor || lastOnPeriod_ms != onPeriod_ms || lastOffPeriod_ms != offPeriod_ms ||
        lastTransitionOnPeriod_ms != transitionOnPeriod_ms || lastTransitionOffPeriod_ms != transitionOffPeriod_ms;
      }
    }

    public virtual void ClearData() {
      onColor = 0;
      offColor = 0;
      onPeriod_ms = 1000;
      offPeriod_ms = 0;
      transitionOnPeriod_ms = 0;
      transitionOffPeriod_ms = 0;

      lastOnColor = 0;
      lastOffColor = 0;
      lastOnPeriod_ms = 1000;
      lastOffPeriod_ms = 0;
      lastTransitionOnPeriod_ms = 0;
      lastTransitionOffPeriod_ms = 0;

      messageDelay = 0f;
    }

    public static float messageDelay = 0f;

    public const uint FOREVER = 2147483647;
    private bool initialized = false;
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

  public Vector3 LastWorldPosition { get; private set; }

  public Quaternion Rotation { get; private set; }

  public Quaternion LastRotation { get; private set; }

  public Vector3 Forward { get { return Rotation * Vector3.right; } }

  public Vector3 Right { get { return Rotation * -Vector3.up; } }

  public RobotStatusFlag RobotStatus { get; private set; }

  public GameStatusFlag GameStatus { get; private set; }

  public float BatteryPercent { get; private set; }

  public List<ObservedObject> VisibleObjects { get; private set; }

  public List<ObservedObject> SeenObjects { get; private set; }

  public List<ObservedObject> DirtyObjects { get; private set; }

  public Dictionary<int, ActiveBlock> ActiveBlocks { get; private set; }

  public List<Face> Faces { get; private set; }

  public Light[] Lights { get; private set; }

  private bool lightsChanged {
    get {
      for (int i = 0; i < Lights.Length; ++i) {
        if (Lights[i].changed)
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

  private int carryingObjectID;

  public int HeadTrackingObjectID { get; private set; }

  private int lastHeadTrackingObjectID;

  private float headAngleRequested;
  private float lastHeadAngleRequestTime;
  private float liftHeightRequested;
  private float lastLiftHeightRequestTime;

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
  private U2G.SetObjectAdditionAndDeletion SetObjectAdditionAndDeletionMessage;
  private U2G.StartFaceTracking StartFaceTrackingMessage;
  private U2G.StopFaceTracking StopFaceTrackingMessage;
  private U2G.ExecuteBehavior ExecuteBehaviorMessage;
  private U2G.SetBehaviorSystemEnabled SetBehaviorSystemEnabledMessage;
  private U2G.ActivateBehaviorChooser ActivateBehaviorChooserMessage;
  private U2G.PlaceRelObject PlaceRelObjectMessage;
  private U2G.QueueCompoundAction QueueCompoundActionsMessage;
  private U2G.PlayAnimation PlayAnimationMessage;
  private U2G.PlayAnimation[] PlayAnimationMessages;
  private U2G.SetIdleAnimation SetIdleAnimationMessage;
  private U2G.SetLiveIdleAnimationParameters SetLiveIdleAnimationParametersMessage;

  private ObservedObject _carryingObject;

  public ObservedObject CarryingObject {
    get {
      if (_carryingObject != carryingObjectID)
        _carryingObject = SeenObjects.Find(x => x == carryingObjectID);

      return _carryingObject;
    }
  }

  private ObservedObject _headTrackingObject;

  public ObservedObject HeadTrackingObject {
    get {
      if (_headTrackingObject != HeadTrackingObjectID)
        _headTrackingObject = SeenObjects.Find(x => x == HeadTrackingObjectID);
      
      return _headTrackingObject;
    }
  }

  public enum ObservedObjectListType {
    KNOWN_IN_RANGE,
    MARKERS_SEEN,
    KNOWN,
    OBSERVED_RECENTLY
  }

  private List<KeyValuePair<RobotActionType, RobotCallback>> robotCallbacks = new List<KeyValuePair<RobotActionType, RobotCallback>>();

  // er, should be 5?
  private const float MaxVoltage = 5.0f;

  [System.NonSerialized] public float localBusyTimer = 0f;
  [System.NonSerialized] public bool localBusyOverride = false;

  public bool isBusy {
    get {
      return localBusyOverride
      || localBusyTimer > 0f
      || Status(RobotStatusFlag.IS_PATHING)
      || (Status(RobotStatusFlag.IS_ANIMATING) && !Status(RobotStatusFlag.IS_ANIMATING_IDLE))
      || Status(RobotStatusFlag.IS_PICKED_UP);
    }

    set {
      localBusyOverride = value;

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
    ActiveBlocks = new Dictionary<int, ActiveBlock>();
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
    SetObjectAdditionAndDeletionMessage = new U2G.SetObjectAdditionAndDeletion();
    StartFaceTrackingMessage = new U2G.StartFaceTracking();
    StopFaceTrackingMessage = new U2G.StopFaceTracking();
    ExecuteBehaviorMessage = new U2G.ExecuteBehavior();
    SetBehaviorSystemEnabledMessage = new U2G.SetBehaviorSystemEnabled();
    ActivateBehaviorChooserMessage = new U2G.ActivateBehaviorChooser();
    PlaceRelObjectMessage = new U2G.PlaceRelObject();
    PlayAnimationMessage = new U2G.PlayAnimation();
    PlayAnimationMessages = new U2G.PlayAnimation[2];
    PlayAnimationMessages[0] = new U2G.PlayAnimation();
    PlayAnimationMessages[1] = new U2G.PlayAnimation();

    SetIdleAnimationMessage = new U2G.SetIdleAnimation();
    SetLiveIdleAnimationParametersMessage = new U2G.SetLiveIdleAnimationParameters();
    QueueCompoundActionsMessage = new U2G.QueueCompoundAction();
    QueueCompoundActionsMessage.actions = new U2G.RobotActionUnion[2];
    QueueCompoundActionsMessage.actions[0] = new U2G.RobotActionUnion();
    QueueCompoundActionsMessage.actions[1] = new U2G.RobotActionUnion();

    QueueCompoundActionsMessage.actionTypes = new Anki.Cozmo.RobotActionType[2];

    Lights = new Light[SetBackpackLEDsMessage.onColor.Length];

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i] = new Light();
    }

    ClearData(true);

    RobotEngineManager.instance.DisconnectedFromClient += Reset;

    RobotEngineManager.instance.SuccessOrFailure += RobotEngineMessages;
  }

  public void Dispose() {
    RobotEngineManager.instance.DisconnectedFromClient -= Reset;
    RobotEngineManager.instance.SuccessOrFailure -= RobotEngineMessages;
  }

  public void CooldownTimers(float delta) {
    if (localBusyTimer > 0f) {
      localBusyTimer -= delta;
    }
  }

  private void RobotEngineMessages(bool success, RobotActionType messageType) {
    DAS.Info("Robot.ActionCallback", "Type = " + messageType + " success = " + success);
    for (int i = 0; i < robotCallbacks.Count; ++i) {
      if (messageType == robotCallbacks[i].Key) {
        robotCallbacks[i].Value(success);
        robotCallbacks.RemoveAt(i);
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
      SetObjectAdditionAndDeletion(true, true);
      DAS.Debug("Robot", "Robot data cleared");
    }

    SeenObjects.Clear();
    VisibleObjects.Clear();
    ActiveBlocks.Clear();
    RobotStatus = RobotStatusFlag.NoneRobotStatusFlag;
    GameStatus = GameStatusFlag.Nothing;
    WorldPosition = Vector3.zero;
    Rotation = Quaternion.identity;
    carryingObjectID = -1;
    HeadTrackingObjectID = -1;
    lastHeadTrackingObjectID = -1;
    targetLockedObject = null;
    _carryingObject = null;
    _headTrackingObject = null;
    headAngleRequested = float.MaxValue;
    liftHeightRequested = float.MaxValue;
    HeadAngle = float.MaxValue;
    PoseAngle = float.MaxValue;
    LeftWheelSpeed = float.MaxValue;
    RightWheelSpeed = float.MaxValue;
    LiftHeight = float.MaxValue;
    BatteryPercent = float.MaxValue;
    LastWorldPosition = Vector3.zero;
    WorldPosition = Vector3.zero;
    LastRotation = Quaternion.identity;
    Rotation = Quaternion.identity;
    localBusyTimer = 0f;

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].ClearData();
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
    BatteryPercent = (message.batteryVoltage / MaxVoltage);
    carryingObjectID = message.carryingObjectID;
    HeadTrackingObjectID = message.headTrackingObjectID;

    if (HeadTrackingObjectID == lastHeadTrackingObjectID)
      lastHeadTrackingObjectID = -1;

    LastWorldPosition = WorldPosition;
    WorldPosition = new Vector3(message.pose_x, message.pose_y, message.pose_z);

    LastRotation = Rotation;
    Rotation = new Quaternion(message.pose_quaternion1, message.pose_quaternion2, message.pose_quaternion3, message.pose_quaternion0);

  }

  public void UpdateLightMessages(bool now = false) {
    if (RobotEngineManager.instance == null || !RobotEngineManager.instance.IsConnected)
      return;

    if (Time.time > ActiveBlock.Light.messageDelay || now) {
      var enumerator = ActiveBlocks.GetEnumerator();

      while (enumerator.MoveNext()) {
        ActiveBlock activeBlock = enumerator.Current.Value;

        if (activeBlock.lightsChanged)
          activeBlock.SetAllLEDs();
      }
    }

    if (Time.time > Light.messageDelay || now) {
      if (lightsChanged)
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

  public void UpdateObservedObjectInfo(G2U.RobotObservedObject message) {

    if (message.objectFamily == Anki.Cozmo.ObjectFamily.Mat) {
      DAS.Warn("Robot", "UpdateObservedObjectInfo received message about the Mat!");
      return;
    }

    if (message.objectFamily == Anki.Cozmo.ObjectFamily.LightCube) {
      AddActiveBlock(ActiveBlocks.ContainsKey(message.objectID) ? ActiveBlocks[message.objectID] : null, message);
    }
    else {
      ObservedObject knownObject = SeenObjects.Find(x => x == message.objectID);

      AddObservedObject(knownObject, message);
    }

    // check to see if we haven't seen some visible objects in a while
    // and clear them out if we haven't.
    ClearVisibleObjects();
  }

  private void AddActiveBlock(ActiveBlock activeBlock, G2U.RobotObservedObject message) {
    if (activeBlock == null) {
      activeBlock = new ActiveBlock(message.objectID, message.objectFamily, message.objectType);

      ActiveBlocks.Add(activeBlock, activeBlock);
      SeenObjects.Add(activeBlock);
    }

    AddObservedObject(activeBlock, message);
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

    RobotEngineManager.instance.Message.DriveWheels = DriveWheelsMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void PlaceObjectOnGroundHere(RobotCallback callback = null) {
    DAS.Debug("Robot", "Place Object " + CarryingObject + " On Ground Here");

    RobotEngineManager.instance.Message.PlaceObjectOnGroundHere = PlaceObjectOnGroundHereMessage;
    RobotEngineManager.instance.SendMessage();

    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_LOW, callback));
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_HIGH, callback));
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

    RobotEngineManager.instance.Message.PlaceRelObject = PlaceRelObjectMessage;
    RobotEngineManager.instance.SendMessage();

    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_LOW, callback));
    }

  }

  public void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN) {
    CancelActionMessage.robotID = ID;
    CancelActionMessage.actionType = actionType;

    DAS.Debug("Robot", "CancelAction actionType(" + actionType + ")");

    RobotEngineManager.instance.Message.CancelAction = CancelActionMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void SendAnimation(string animName, RobotCallback callback = null) {

    DAS.Debug("CozmoEmotionManager", "Sending " + animName + " with " + 1 + " loop");
    PlayAnimationMessage.animationName = animName;
    PlayAnimationMessage.numLoops = 1;
    PlayAnimationMessage.robotID = this.ID;

    RobotEngineManager.instance.SendMessage();

    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLAY_ANIMATION, callback));
    }
  }

  public void SetIdleAnimation(string default_anim) {
    SetIdleAnimationMessage.animationName = default_anim;
    SetIdleAnimationMessage.robotID = this.ID;
    RobotEngineManager.instance.Message.SetIdleAnimation = SetIdleAnimationMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues) {

    SetLiveIdleAnimationParametersMessage.paramNames = paramNames;
    SetLiveIdleAnimationParametersMessage.paramValues = paramValues;
    SetLiveIdleAnimationParametersMessage.robotID = 1;

    RobotEngineManager.instance.Message.SetLiveIdleAnimationParameters = SetLiveIdleAnimationParametersMessage;
    RobotEngineManager.instance.SendMessage();

  }

  public float GetHeadAngleFactor() {

    float angle = IsHeadAngleRequestUnderway() ? headAngleRequested : HeadAngle;

    if (angle >= 0f) {
      angle = Mathf.Lerp(0f, 1f, angle / (CozmoUtil.MAX_HEAD_ANGLE * Mathf.Deg2Rad));
    }
    else {
      angle = Mathf.Lerp(0f, -1f, angle / (CozmoUtil.MIN_HEAD_ANGLE * Mathf.Deg2Rad));
    }

    return angle;
  }


  public bool IsHeadAngleRequestUnderway() {
    return Time.time < lastHeadAngleRequestTime + CozmoUtil.HEAD_ANGLE_REQUEST_TIME;
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

    if (IsHeadAngleRequestUnderway() && Mathf.Abs(headAngleRequested - radians) < 0.001f)
      return;
    if (HeadTrackingObject == -1 && Mathf.Abs(radians - HeadAngle) < 0.001f)
      return;

    headAngleRequested = radians;
    lastHeadAngleRequestTime = Time.time;

    SetHeadAngleMessage.angle_rad = radians;

    SetHeadAngleMessage.accel_rad_per_sec2 = accelRadSec;
    SetHeadAngleMessage.max_speed_rad_per_sec = maxSpeedFactor * CozmoUtil.MAX_SPEED_RAD_PER_SEC;

    RobotEngineManager.instance.Message.SetHeadAngle = SetHeadAngleMessage;
    RobotEngineManager.instance.SendMessage();

    if (onComplete != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.MOVE_HEAD_TO_ANGLE, onComplete));
    }
  }

  public void TrackToObject(ObservedObject observedObject, bool headOnly = true) {
    if (HeadTrackingObjectID == observedObject) {
      lastHeadTrackingObjectID = -1;
      return;
    }
    else if (lastHeadTrackingObjectID == observedObject) {
      return;
    }

    lastHeadTrackingObjectID = observedObject;

    if (observedObject != null) {
      TrackToObjectMessage.objectID = (uint)observedObject;
    }
    else {
      TrackToObjectMessage.objectID = uint.MaxValue; //cancels tracking
    }

    TrackToObjectMessage.robotID = ID;
    TrackToObjectMessage.headOnly = headOnly;

    DAS.Debug("Robot", "Track Head To Object " + TrackToObjectMessage.objectID);

    RobotEngineManager.instance.Message.TrackToObject = TrackToObjectMessage;
    RobotEngineManager.instance.SendMessage();

  }

  public void FaceObject(ObservedObject observedObject, bool headTrackWhenDone = true) {
    FaceObjectMessage.objectID = observedObject;
    FaceObjectMessage.robotID = ID;
    FaceObjectMessage.maxTurnAngle = float.MaxValue;
    FaceObjectMessage.turnAngleTol = Mathf.Deg2Rad; //one degree seems to work?
    FaceObjectMessage.headTrackWhenDone = System.Convert.ToByte(headTrackWhenDone);
    
    DAS.Debug("Robot", "Face Object " + FaceObjectMessage.objectID);

    RobotEngineManager.instance.Message.FaceObject = FaceObjectMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void FacePose(Face face) {
    FacePoseMessage.maxTurnAngle = float.MaxValue;
    FacePoseMessage.robotID = ID;
    FacePoseMessage.turnAngleTol = Mathf.Deg2Rad; //one degree seems to work?
    FacePoseMessage.world_x = face.WorldPosition.x;
    FacePoseMessage.world_y = face.WorldPosition.y;
    FacePoseMessage.world_z = face.WorldPosition.z;

    RobotEngineManager.instance.Message.FacePose = FacePoseMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void PickupObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, RobotCallback callback = null) {
    PickupObjectMessage.objectID = selectedObject;
    PickupObjectMessage.usePreDockPose = usePreDockPose;
    PickupObjectMessage.useManualSpeed = useManualSpeed;
    PickupObjectMessage.useApproachAngle = useApproachAngle;
    PickupObjectMessage.approachAngle_rad = approachAngleRad;
    
    DAS.Debug("Robot", "Pick And Place Object " + PickupObjectMessage.objectID + " usePreDockPose " + PickupObjectMessage.usePreDockPose + " useManualSpeed " + PickupObjectMessage.useManualSpeed);

    RobotEngineManager.instance.Message.PickupObject = PickupObjectMessage;
    RobotEngineManager.instance.SendMessage();

    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;

    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PICKUP_OBJECT_LOW, callback));
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PICK_AND_PLACE_INCOMPLETE, callback));
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PICKUP_OBJECT_HIGH, callback));
    }
  }

  public void RollObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, RobotCallback callback = null) {
    RollObjectMessage.objectID = selectedObject;
    RollObjectMessage.usePreDockPose = usePreDockPose;
    RollObjectMessage.useManualSpeed = useManualSpeed;

    DAS.Debug("Robot", "Roll Object " + RollObjectMessage.objectID + " usePreDockPose " + RollObjectMessage.usePreDockPose + " useManualSpeed " + RollObjectMessage.useManualSpeed);
    
    RobotEngineManager.instance.Message.RollObject = RollObjectMessage;
    RobotEngineManager.instance.SendMessage();

    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.ROLL_OBJECT_LOW, callback));
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
    
    DAS.Debug("Robot", "Drop Object At Pose " + position + " useManualSpeed " + useManualSpeed);
    
    RobotEngineManager.instance.Message.PlaceObjectOnGround = PlaceObjectOnGroundMessage;
    RobotEngineManager.instance.SendMessage();
    
    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;

    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.PLACE_OBJECT_LOW, callback));
    }

  }

  public Vector3 NudgePositionOutOfObjects(Vector3 position) {

    int attempts = 0;

    while (attempts++ < 3) {
      Vector3 nudge = Vector3.zero;
      float padding = CozmoUtil.BLOCK_LENGTH_MM * 2f;
      for (int i = 0; i < SeenObjects.Count; i++) {
        Vector3 fromObject = position - SeenObjects[i].WorldPosition;
        if (fromObject.magnitude > padding)
          continue;
        nudge += fromObject.normalized * padding;
      }

      if (nudge.sqrMagnitude == 0f)
        break;
      position += nudge;
    }

    return position;
  }

  public void GotoPose(float x_mm, float y_mm, float rad, RobotCallback callback = null, bool level = false, bool useManualSpeed = false) {
    GotoPoseMessage.level = System.Convert.ToByte(level);
    GotoPoseMessage.useManualSpeed = useManualSpeed;
    GotoPoseMessage.x_mm = x_mm;
    GotoPoseMessage.y_mm = y_mm;
    GotoPoseMessage.rad = rad;

    DAS.Debug("Robot", "Go to Pose: x: " + GotoPoseMessage.x_mm + " y: " + GotoPoseMessage.y_mm + " useManualSpeed: " + GotoPoseMessage.useManualSpeed + " level: " + GotoPoseMessage.level);

    RobotEngineManager.instance.Message.GotoPose = GotoPoseMessage;
    RobotEngineManager.instance.SendMessage();
    
    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;

    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.DRIVE_TO_POSE, callback));
    }
  }

  public void GotoObject(ObservedObject obj, float distance_mm, RobotCallback callback = null) {
    GotoObjectMessage.objectID = obj;
    GotoObjectMessage.distance_mm = distance_mm;
    GotoObjectMessage.useManualSpeed = false;

    RobotEngineManager.instance.Message.GotoObject = GotoObjectMessage;

    RobotEngineManager.instance.SendMessage();
    
    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.DRIVE_TO_OBJECT, callback));
    }
  }

  public float GetLiftHeightFactor() {
    return (Time.time < lastLiftHeightRequestTime + CozmoUtil.LIFT_REQUEST_TIME) ? liftHeightRequested : LiftHeightFactor;
  }

  public void SetLiftHeight(float height_factor, RobotCallback callback = null) {
    if ((Time.time < lastLiftHeightRequestTime + CozmoUtil.LIFT_REQUEST_TIME && height_factor == liftHeightRequested) || LiftHeightFactor == height_factor)
      return;

    liftHeightRequested = height_factor;
    lastLiftHeightRequestTime = Time.time;

    SetLiftHeightMessage.accel_rad_per_sec2 = 5f;
    SetLiftHeightMessage.max_speed_rad_per_sec = 10f;
    SetLiftHeightMessage.height_mm = (height_factor * (CozmoUtil.MAX_LIFT_HEIGHT_MM - CozmoUtil.MIN_LIFT_HEIGHT_MM)) + CozmoUtil.MIN_LIFT_HEIGHT_MM;

    RobotEngineManager.instance.Message.SetLiftHeight = SetLiftHeightMessage;
    RobotEngineManager.instance.SendMessage();

    if (callback != null) {
      robotCallbacks.Add(new KeyValuePair<RobotActionType, RobotCallback>(RobotActionType.MOVE_LIFT_TO_HEIGHT, callback));
    }
  }

  public void SetRobotCarryingObject(int objectID = -1) {
    DAS.Debug("Robot", "Set Robot Carrying Object: " + objectID);
    
    SetRobotCarryingObjectMessage.robotID = ID;
    SetRobotCarryingObjectMessage.objectID = objectID;

    RobotEngineManager.instance.Message.SetRobotCarryingObject = SetRobotCarryingObjectMessage;
    RobotEngineManager.instance.SendMessage();

    targetLockedObject = null;
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  public void ClearAllBlocks() {
    DAS.Debug("Robot", "Clear All Blocks");
    ClearAllBlocksMessage.robotID = ID;
    RobotEngineManager.instance.Message.ClearAllBlocks = ClearAllBlocksMessage;
    RobotEngineManager.instance.SendMessage();
    Reset();
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  public void ClearAllObjects() {
    ClearAllObjectsMessage.robotID = ID;
    RobotEngineManager.instance.Message.ClearAllObjects = ClearAllObjectsMessage;
    RobotEngineManager.instance.SendMessage();
    Reset();
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  
  public void VisionWhileMoving(bool enable) {
    VisionWhileMovingMessage.enable = System.Convert.ToByte(enable);

    RobotEngineManager.instance.Message.VisionWhileMoving = VisionWhileMovingMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void StopAllMotors() {
    RobotEngineManager.instance.SendMessage();
  }

  public void TurnInPlace(float angle_rad) {
    TurnInPlaceMessage.robotID = ID;
    TurnInPlaceMessage.angle_rad = angle_rad;

    RobotEngineManager.instance.Message.TurnInPlace = TurnInPlaceMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false) {
    DAS.Debug("Robot", "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose);

    TraverseObjectMessage.useManualSpeed = useManualSpeed;
    TraverseObjectMessage.usePreDockPose = usePreDockPose;

    RobotEngineManager.instance.Message.TraverseObject = TraverseObjectMessage;
    RobotEngineManager.instance.SendMessage();

    localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
  }

  private void SetLastLEDs() {
    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].SetLastInfo();
    }
  }

  public void SetBackpackLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                              uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {
    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].onColor = onColor;
      Lights[i].offColor = offColor;
      Lights[i].onPeriod_ms = onPeriod_ms;
      Lights[i].offPeriod_ms = offPeriod_ms;
      Lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms;
      Lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms;
    }
  }

  private void SetAllBackpackLEDs() { // should only be called from update loop
    //Debug.Log ("frame("+Time.frameCount+") SetAllBackpackLEDs " + lights[0].onColor);

    SetBackpackLEDsMessage.robotID = ID;

    for (int i = 0; i < Lights.Length; ++i) {
      SetBackpackLEDsMessage.onColor[i] = Lights[i].onColor;
      SetBackpackLEDsMessage.offColor[i] = Lights[i].offColor;
      SetBackpackLEDsMessage.onPeriod_ms[i] = Lights[i].onPeriod_ms;
      SetBackpackLEDsMessage.offPeriod_ms[i] = Lights[i].offPeriod_ms;
      SetBackpackLEDsMessage.transitionOnPeriod_ms[i] = Lights[i].transitionOnPeriod_ms;
      SetBackpackLEDsMessage.transitionOffPeriod_ms[i] = Lights[i].transitionOffPeriod_ms;
    }
    
    RobotEngineManager.instance.Message.SetBackpackLEDs = SetBackpackLEDsMessage;
    RobotEngineManager.instance.SendMessage();

    SetLastLEDs();
  }

  public void SetObjectAdditionAndDeletion(bool enableAddition, bool enableDeletion) {
    if (RobotEngineManager.instance == null || !RobotEngineManager.instance.IsConnected)
      return;
    
    SetObjectAdditionAndDeletionMessage.robotID = ID;
    SetObjectAdditionAndDeletionMessage.enableAddition = enableAddition;
    SetObjectAdditionAndDeletionMessage.enableDeletion = enableDeletion;

    RobotEngineManager.instance.Message.SetObjectAdditionAndDeletion = SetObjectAdditionAndDeletionMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void StartFaceAwareness() {
    StartFaceTrackingMessage.timeout_sec = byte.MaxValue;

    RobotEngineManager.instance.Message.StartFaceTracking = StartFaceTrackingMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void StopFaceAwareness() {
    RobotEngineManager.instance.Message.StopFaceTracking = StopFaceTrackingMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void TurnOffAllLights(bool now = false) {
    var enumerator = ActiveBlocks.GetEnumerator();
    
    while (enumerator.MoveNext()) {
      ActiveBlock activeBlock = enumerator.Current.Value;
      
      activeBlock.SetLEDs();
    }

    SetBackpackLEDs();

    if (now)
      UpdateLightMessages(now);
  }

  public void ExecuteBehavior(BehaviorType type) {
    ExecuteBehaviorMessage.behaviorType = type;
    
    DAS.Debug("Robot", "Execute Behavior " + ExecuteBehaviorMessage.behaviorType);

    RobotEngineManager.instance.Message.ExecuteBehavior = ExecuteBehaviorMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void SetBehaviorSystem(bool enable) {
    SetBehaviorSystemEnabledMessage.enabled = enable;

    RobotEngineManager.instance.Message.SetBehaviorSystemEnabled = SetBehaviorSystemEnabledMessage;
    RobotEngineManager.instance.SendMessage();
  }

  public void ActivateBehaviorChooser(BehaviorChooserType behaviorChooserType) {
    ActivateBehaviorChooserMessage.behaviorChooserType = behaviorChooserType;

    RobotEngineManager.instance.Message.ActivateBehaviorChooser = ActivateBehaviorChooserMessage;
    RobotEngineManager.instance.SendMessage();
  }
}
