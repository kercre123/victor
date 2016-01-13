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

  private struct RobotCallbackWrapper {
    public readonly uint IdTag;

    private RobotCallback _Callback;

    public event RobotCallback Callback { add {
        _Callback += value;
      }
      remove {
        _Callback -= value;
      }
    }

    public RobotCallbackWrapper(uint idTag, RobotCallback callback) {
      IdTag = idTag;
      _Callback = callback;
    }

    public void Invoke(bool success) {
      if (_Callback != null) {
        var target = _Callback.Target;

        // Unity Overrides the == operator to return null if the object has been
        // destroyed, but the callback will still be registered.
        // By casting to UnityEngine.Object, we can test if it has been destroyed
        // and not invoke the callback.
        if (target is UnityEngine.Object) {

          if (((UnityEngine.Object)target) == null) {
            DAS.Error(this, "Tried to invoke callback on destroyed object of type " + target.GetType().Name);
            return;
          }
        }

        _Callback(success);
      }
    }
  }

  public byte ID { get; private set; }

  // in radians
  public float HeadAngle { get; private set; }

  // robot yaw in radians, from negative PI to positive PI
  public float PoseAngle { get; private set; }

  public float PitchAngle { get; private set; }

  // in mm/s
  public float LeftWheelSpeed { get; private set; }

  // in mm/s
  public float RightWheelSpeed { get; private set; }

  // in mm
  public float LiftHeight { get; private set; }

  public float LiftHeightFactor { get { return (LiftHeight - CozmoUtil.kMinLiftHeightMM) / (CozmoUtil.kMaxLiftHeightMM - CozmoUtil.kMinLiftHeightMM); } }

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

  public int[] ProgressionStats { get; private set; }

  public int FriendshipPoints { get; private set; }

  public int FriendshipLevel { get; private set; }

  public float[] EmotionValues { get; private set; }

  public Light[] BackpackLights { get; private set; }

  private bool _LightsChanged {
    get {
      for (int i = 0; i < BackpackLights.Length; ++i) {
        if (BackpackLights[i].Changed)
          return true;
      }

      return false;
    }
  }

  private ObservedObject _targetLockedObject = null;

  public ObservedObject TargetLockedObject {
    get { return _targetLockedObject; }
    set {
      _targetLockedObject = value;
    }
  }

  private int _CarryingObjectID;

  public int CarryingObjectID { 
    get { return _CarryingObjectID; } 
    private set { 
      if (_CarryingObjectID != value) {
        _CarryingObjectID = value; 
        if (OnCarryingObjectSet != null) {
          OnCarryingObjectSet(CarryingObject);
        }
      }
    } 
  }

  private int _HeadTrackingObjectID;

  public int HeadTrackingObjectID { 
    get { return _HeadTrackingObjectID; } 
    private set { 
      if (_HeadTrackingObjectID != value) {
        _HeadTrackingObjectID = value; 
        if (OnHeadTrackingObjectSet != null) {
          OnHeadTrackingObjectSet(HeadTrackingObject);
        }
      }
    } 
  }

  private int _LastHeadTrackingObjectID;

  public string CurrentBehaviorString { get; set; }

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
  private U2G.PlaceOnObject PlaceOnObjectMessage;
  private U2G.PlayAnimation PlayAnimationMessage;
  private U2G.SetIdleAnimation SetIdleAnimationMessage;
  private U2G.SetLiveIdleAnimationParameters SetLiveIdleAnimationParametersMessage;
  private U2G.SetRobotVolume SetRobotVolumeMessage;
  private U2G.AlignWithObject AlignWithObjectMessage;
  private U2G.ProgressionMessage ProgressionStatMessage;
  private U2G.SetFriendshipPoints FriendshipPointsMessage;
  private U2G.SetFriendshipLevel FriendshipLevelMessage;
  private U2G.MoodMessage MoodStatMessage;
  private U2G.VisualizeQuad VisualizeQuadMessage;
  private U2G.DisplayProceduralFace DisplayProceduralFaceMessage;
  private U2G.QueueSingleAction QueueSingleAction;

  private PathMotionProfile PathMotionProfileDefault;

  private uint _LastIdTag;

  private ObservedObject _CarryingObject;

  public ObservedObject CarryingObject {
    get {
      if (_CarryingObject != CarryingObjectID)
        _CarryingObject = SeenObjects.Find(x => x == CarryingObjectID);

      return _CarryingObject;
    }
  }

  public event Action<ObservedObject> OnCarryingObjectSet;


  private ObservedObject _HeadTrackingObject;

  public ObservedObject HeadTrackingObject {
    get {
      if (_HeadTrackingObject != HeadTrackingObjectID)
        _HeadTrackingObject = SeenObjects.Find(x => x == HeadTrackingObjectID);
      
      return _HeadTrackingObject;
    }
  }

  public event Action<ObservedObject> OnHeadTrackingObjectSet;

  private readonly List<RobotCallbackWrapper> _RobotCallbacks = new List<RobotCallbackWrapper>();

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
    PlaceOnObjectMessage = new U2G.PlaceOnObject();
    PlayAnimationMessage = new U2G.PlayAnimation();
    SetRobotVolumeMessage = new U2G.SetRobotVolume();
    AlignWithObjectMessage = new U2G.AlignWithObject();
    ProgressionStatMessage = new U2G.ProgressionMessage();
    MoodStatMessage = new U2G.MoodMessage();
    VisualizeQuadMessage = new U2G.VisualizeQuad();
    DisplayProceduralFaceMessage = new U2G.DisplayProceduralFace();
    QueueSingleAction = new U2G.QueueSingleAction();
    QueueSingleAction.robotID = ID;
    FriendshipPointsMessage = new U2G.SetFriendshipPoints();
    FriendshipLevelMessage = new U2G.SetFriendshipLevel();

    // These defaults should eventually be in clad
    PathMotionProfileDefault = new PathMotionProfile();
    PathMotionProfileDefault.speed_mmps = 60.0f;
    PathMotionProfileDefault.accel_mmps2 = 200.0f;
    PathMotionProfileDefault.decel_mmps2 = 500.0f;
    PathMotionProfileDefault.pointTurnSpeed_rad_per_sec = 2.0f;
    PathMotionProfileDefault.pointTurnAccel_rad_per_sec2 = 100.0f;
    PathMotionProfileDefault.pointTurnDecel_rad_per_sec2 = 500.0f;
    PathMotionProfileDefault.dockSpeed_mmps = 100.0f;
    PathMotionProfileDefault.dockAccel_mmps2 = 200.0f;

    SetIdleAnimationMessage = new U2G.SetIdleAnimation();
    SetLiveIdleAnimationParametersMessage = new U2G.SetLiveIdleAnimationParameters();

    BackpackLights = new Light[SetBackpackLEDsMessage.onColor.Length];

    ProgressionStats = new int[(int)Anki.Cozmo.ProgressionStatType.Count];
    EmotionValues = new float[(int)Anki.Cozmo.EmotionType.Count];

    for (int i = 0; i < BackpackLights.Length; ++i) {
      BackpackLights[i] = new Light();
    }

    ClearData(true);

    RobotEngineManager.Instance.DisconnectedFromClient += Reset;

    RobotEngineManager.Instance.SuccessOrFailure += RobotEngineMessages;
    RobotEngineManager.Instance.OnEmotionRecieved += UpdateEmotionFromEngineRobotManager;
    RobotEngineManager.Instance.OnProgressionStatRecieved += UpdateProgressionStatFromEngineRobotManager;
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

  public Vector3 WorldToCozmo(Vector3 worldSpacePosition) {
    Vector3 offset = worldSpacePosition - this.WorldPosition;
    offset = Quaternion.Inverse(this.Rotation) * offset;
    return offset;
  }

  private void RobotEngineMessages(uint idTag, bool success, RobotActionType messageType) {
    DAS.Info("Robot.ActionCallback", "Type = " + messageType + " success = " + success);

    for (int i = 0; i < _RobotCallbacks.Count; ++i) {
      if (_RobotCallbacks[i].IdTag == idTag) {
        _RobotCallbacks[i].Invoke(success);
        _RobotCallbacks.RemoveAt(i);
        break;
      }
    }
  }

  private uint GetNextIdTag() {
    return ++_LastIdTag;
  }

  private int SortByDistance(ObservedObject a, ObservedObject b) {
    float d1 = ((Vector2)a.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
    float d2 = ((Vector2)b.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
    return d1.CompareTo(d2);
  }

  private void Reset(DisconnectionReason reason = DisconnectionReason.None) {
    ClearData();
  }

  public void ResetRobotState() {
    DriveWheels(0.0f, 0.0f);
    SetHeadAngle(0.0f);
    SetLiftHeight(0.0f);
    TrackToObject(null);
    CancelAllCallbacks();
    SetBehaviorSystem(false);
    ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    SetIdleAnimation("NONE");
    Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = { };
    float[] paramValues = { };
    SetLiveIdleAnimationParameters(paramNames, paramValues, true);
    foreach (KeyValuePair<int, LightCube> kvp in LightCubes) {
      kvp.Value.SetLEDs(Color.black);
    }
    SetBackpackLEDs(CozmoPalette.ColorToUInt(Color.black));
  }

  public void ClearData(bool initializing = false) {
    if (!initializing) {
      TurnOffAllLights(true);
      DAS.Debug(this, "Robot data cleared");
    }

    SeenObjects.Clear();
    VisibleObjects.Clear();
    LightCubes.Clear();
    RobotStatus = RobotStatusFlag.NoneRobotStatusFlag;
    GameStatus = GameStatusFlag.Nothing;
    WorldPosition = Vector3.zero;
    Rotation = Quaternion.identity;
    CarryingObjectID = -1;
    HeadTrackingObjectID = -1;
    _LastHeadTrackingObjectID = -1;
    TargetLockedObject = null;
    _CarryingObject = null;
    _HeadTrackingObject = null;
    HeadAngle = float.MaxValue;
    PoseAngle = float.MaxValue;
    LeftWheelSpeed = float.MaxValue;
    RightWheelSpeed = float.MaxValue;
    LiftHeight = float.MaxValue;
    BatteryPercent = float.MaxValue;
    LocalBusyTimer = 0f;

    for (int i = 0; i < BackpackLights.Length; ++i) {
      BackpackLights[i].ClearData();
    }

    for (int i = 0; i < (int)Anki.Cozmo.ProgressionStatType.Count; ++i) {
      ProgressionStats[i] = 0;
    }
  }

  public void ClearVisibleObjects() {
    for (int i = 0; i < VisibleObjects.Count; ++i) {
      if (VisibleObjects[i].TimeLastSeen + ObservedObject.kRemoveDelay < Time.time) {
        VisibleObjects.RemoveAt(i--);
      }
    }
  }

  public void UpdateInfo(G2U.RobotState message) {
    HeadAngle = message.headAngle_rad;
    PoseAngle = message.poseAngle_rad;
    PitchAngle = message.posePitch_rad;
    LeftWheelSpeed = message.leftWheelSpeed_mmps;
    RightWheelSpeed = message.rightWheelSpeed_mmps;
    LiftHeight = message.liftHeight_mm;
    RobotStatus = (RobotStatusFlag)message.status;
    GameStatus = (GameStatusFlag)message.gameStatus;
    BatteryPercent = (message.batteryVoltage / CozmoUtil.kMaxVoltage);
    CarryingObjectID = message.carryingObjectID;
    HeadTrackingObjectID = message.headTrackingObjectID;

    WorldPosition = new Vector3(message.pose_x, message.pose_y, message.pose_z);
    Rotation = new Quaternion(message.pose_qx, message.pose_qy, message.pose_qz, message.pose_qw);

  }

  // Object moved, so remove it from the seen list
  // and add it into the dirty list.
  public void UpdateDirtyList(ObservedObject dirty) {
    SeenObjects.Remove(dirty);

    if (!DirtyObjects.Contains(dirty)) {
      DirtyObjects.Add(dirty);
    }
  }

  #region Friendship Points

  public void AddToFriendshipPoints(int deltaValue) {
    FriendshipPoints += deltaValue;
    FriendshipPointsMessage.newVal = FriendshipPoints;
    RobotEngineManager.Instance.Message.SetFriendshipPoints = FriendshipPointsMessage;
    RobotEngineManager.Instance.SendMessage();

    ComputeFriendshipLevel();

    FriendshipLevelMessage.newVal = FriendshipLevel;
    RobotEngineManager.Instance.Message.SetFriendshipLevel = FriendshipLevelMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  private void ComputeFriendshipLevel() {
    FriendshipLevelConfig levelConfig = RobotEngineManager.Instance.GetFriendshipLevelConfig();
    FriendshipLevelConfig.FriendshipLevelData friendshipLevel = null;
    int i = 0;
    for (; i < levelConfig.FriendshipLevels.Length; ++i) {
      if (levelConfig.FriendshipLevels[i].LevelRequirement > FriendshipPoints) {
        break;
      }
    }
    FriendshipLevel = i - 1;
  }

  #endregion

  #region Progression and Mood Stats

  public void AddToProgressionStat(Anki.Cozmo.ProgressionStatType index, int deltaValue) {
    ProgressionStatMessage.robotID = ID;
    ProgressionStatMessage.ProgressionMessageUnion.AddToProgressionStat.statType = index;
    ProgressionStatMessage.ProgressionMessageUnion.AddToProgressionStat.deltaVal = deltaValue;

    RobotEngineManager.Instance.Message.ProgressionMessage = ProgressionStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void VisualizeQuad(Vector3 lowerLeft, Vector3 upperRight) {
    VisualizeQuadMessage.color = CozmoPalette.ColorToUInt(Color.black);
    VisualizeQuadMessage.xLowerLeft = lowerLeft.x;
    VisualizeQuadMessage.yLowerLeft = lowerLeft.y;
    VisualizeQuadMessage.zLowerLeft = lowerLeft.z;

    VisualizeQuadMessage.xUpperRight = upperRight.x;
    VisualizeQuadMessage.yUpperRight = upperRight.y;
    VisualizeQuadMessage.zUpperRight = upperRight.z;

    RobotEngineManager.Instance.Message.VisualizeQuad = VisualizeQuadMessage;
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
    // We can't call SetEmotion.Type because that would call a Getter on SetEmotion.
    MoodStatMessage.MoodMessageUnion.SetEmotion = new G2U.SetEmotion(type, value);

    RobotEngineManager.Instance.Message.MoodMessage = MoodStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  // Only debug panels should be using set.
  // You should not be calling this from a minigame/challenge.
  public void SetProgressionStat(Anki.Cozmo.ProgressionStatType type, int value) {
    ProgressionStatMessage.robotID = ID;
    ProgressionStatMessage.ProgressionMessageUnion.SetProgressionStat = new G2U.SetProgressionStat(type, value);

    RobotEngineManager.Instance.Message.ProgressionMessage = ProgressionStatMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  private void UpdateProgressionStatFromEngineRobotManager(Anki.Cozmo.ProgressionStatType index, int value) {
    ProgressionStats[(int)index] = value;
  }

  private void UpdateEmotionFromEngineRobotManager(Anki.Cozmo.EmotionType index, float value) {
    EmotionValues[(int)index] = value;
  }

  #endregion

  public void UpdateObservedObjectInfo(G2U.RobotObservedObject message) {
    if (message.objectFamily == Anki.Cozmo.ObjectFamily.Mat) {
      DAS.Warn(this, "UpdateObservedObjectInfo received message about the Mat!");
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

  public void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams) {

    //TODO: We should be displaying whatever is on the face on the robot here, but
    // we don't have access to that yet.
    CozmoFace.DisplayProceduralFace(faceAngle, faceCenter, faceScale, leftEyeParams, rightEyeParams);

    DisplayProceduralFaceMessage.robotID = ID;
    DisplayProceduralFaceMessage.faceAngle = faceAngle;
    DisplayProceduralFaceMessage.faceCenX = faceCenter.x;
    DisplayProceduralFaceMessage.faceCenY = faceCenter.y;
    DisplayProceduralFaceMessage.faceScaleX = faceScale.x;
    DisplayProceduralFaceMessage.faceScaleY = faceScale.y;
    DisplayProceduralFaceMessage.leftEye = leftEyeParams;
    DisplayProceduralFaceMessage.rightEye = rightEyeParams;

    RobotEngineManager.Instance.Message.DisplayProceduralFace = DisplayProceduralFaceMessage;
    RobotEngineManager.Instance.SendMessage();

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

  public void PlaceObjectOnGroundHere(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "Place Object " + CarryingObject + " On Ground Here");

    QueueSingleAction.action.placeObjectOnGroundHere = PlaceObjectOnGroundHereMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.kLocalBusyTime;
    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void PlaceObjectRel(ObservedObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "PlaceObjectRel " + target.ID);

    PlaceRelObjectMessage.approachAngle_rad = approachAngle;
    PlaceRelObjectMessage.placementOffsetX_mm = offsetFromMarker;
    PlaceRelObjectMessage.objectID = target.ID;
    PlaceRelObjectMessage.useApproachAngle = true;
    PlaceRelObjectMessage.usePreDockPose = true;
    PlaceRelObjectMessage.useManualSpeed = false;
    PlaceRelObjectMessage.motionProf = PathMotionProfileDefault;

    QueueSingleAction.action.placeRelObject = PlaceRelObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void PlaceOnObject(ObservedObject target, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "PlaceOnObject " + target.ID);

    PlaceOnObjectMessage.approachAngle_rad = approachAngle;
    PlaceOnObjectMessage.objectID = target.ID;
    PlaceOnObjectMessage.useApproachAngle = true;
    PlaceOnObjectMessage.usePreDockPose = true;
    PlaceOnObjectMessage.useManualSpeed = false;
    PlaceOnObjectMessage.motionProf = PathMotionProfileDefault;

    QueueSingleAction.action.placeOnObject = PlaceOnObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;

    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN) {
    CancelActionMessage.robotID = ID;
    CancelActionMessage.actionType = actionType;

    DAS.Debug(this, "CancelAction actionType(" + actionType + ")");

    RobotEngineManager.Instance.Message.CancelAction = CancelActionMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void CancelCallback(RobotCallback callback) {
    for (int i = _RobotCallbacks.Count - 1; i >= 0; i--) {
      _RobotCallbacks[i].Callback -= callback;
    }
  }

  public void CancelAllCallbacks() {
    _RobotCallbacks.Clear();
  }

  public void SendAnimation(string animName, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    DAS.Debug(this, "Sending " + animName + " with " + 1 + " loop");
    // TODO: We should be displaying what is actually on the robot, instead
    // we are faking it.
    CozmoFace.PlayAnimation(animName);

    PlayAnimationMessage.animationName = animName;
    PlayAnimationMessage.numLoops = 1;
    PlayAnimationMessage.robotID = this.ID;

    QueueSingleAction.action.playAnimation = PlayAnimationMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void SetIdleAnimation(string default_anim) {
    SetIdleAnimationMessage.animationName = default_anim;
    SetIdleAnimationMessage.robotID = this.ID;
    RobotEngineManager.Instance.Message.SetIdleAnimation = SetIdleAnimationMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues, bool setUnspecifiedToDefault = false) {

    SetLiveIdleAnimationParametersMessage.paramNames = paramNames;
    SetLiveIdleAnimationParametersMessage.paramValues = paramValues;
    SetLiveIdleAnimationParametersMessage.robotID = 1;
    SetLiveIdleAnimationParametersMessage.setUnspecifiedToDefault = setUnspecifiedToDefault;

    RobotEngineManager.Instance.Message.SetLiveIdleAnimationParameters = SetLiveIdleAnimationParametersMessage;
    RobotEngineManager.Instance.SendMessage();

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

  /// <summary>
  /// Sets the head angle.
  /// </summary>
  /// <param name="angleFactor">Angle factor.</param> usually from -1 (MIN_HEAD_ANGLE) to 1 (kMaxHeadAngle)
  /// <param name="useExactAngle">If set to <c>true</c> angleFactor is treated as an exact angle in radians.</param>
  public void SetHeadAngle(float angleFactor = -0.8f, 
                           Robot.RobotCallback callback = null,
                           QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                           bool useExactAngle = false, 
                           float accelRadSec = 2f, 
                           float maxSpeedFactor = 1f) {

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

    SetHeadAngleMessage.angle_rad = radians;

    SetHeadAngleMessage.accel_rad_per_sec2 = accelRadSec;
    SetHeadAngleMessage.max_speed_rad_per_sec = maxSpeedFactor * CozmoUtil.kMaxSpeedRadPerSec;

    QueueSingleAction.action.setHeadAngle = SetHeadAngleMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 1;

    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;

    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void SetRobotVolume(float volume) {
    SetRobotVolumeMessage.volume = volume;
    DAS.Debug(this, "Set Robot Volume " + SetRobotVolumeMessage.volume);

    RobotEngineManager.Instance.Message.SetRobotVolume = SetRobotVolumeMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public float GetRobotVolume() {
    return SetRobotVolumeMessage.volume;
  }

  public void TrackToObject(ObservedObject observedObject, bool headOnly = true) {
    if (HeadTrackingObjectID == observedObject && _LastHeadTrackingObjectID == observedObject) {
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

    DAS.Debug(this, "Track Head To Object " + TrackToObjectMessage.objectID);

    RobotEngineManager.Instance.Message.TrackToObject = TrackToObjectMessage;
    RobotEngineManager.Instance.SendMessage();

  }

  public void StopTrackToObject() {
    TrackToObject(null);
  }

  public void FaceObject(ObservedObject observedObject, bool headTrackWhenDone = true,
                         Robot.RobotCallback callback = null,
                         QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    FaceObjectMessage.objectID = observedObject;
    FaceObjectMessage.robotID = ID;
    FaceObjectMessage.maxTurnAngle = float.MaxValue;
    FaceObjectMessage.turnAngleTol = Mathf.Deg2Rad; //one degree seems to work?
    FaceObjectMessage.headTrackWhenDone = System.Convert.ToByte(headTrackWhenDone);
    
    DAS.Debug(this, "Face Object " + FaceObjectMessage.objectID);

    QueueSingleAction.action.faceObject = FaceObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;

    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
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

  public void PickupObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    PickupObjectMessage.objectID = selectedObject;
    PickupObjectMessage.usePreDockPose = usePreDockPose;
    PickupObjectMessage.useManualSpeed = useManualSpeed;
    PickupObjectMessage.useApproachAngle = useApproachAngle;
    PickupObjectMessage.approachAngle_rad = approachAngleRad;
    PickupObjectMessage.motionProf = PathMotionProfileDefault;
    
    DAS.Debug(this, "Pick And Place Object " + PickupObjectMessage.objectID + " usePreDockPose " + PickupObjectMessage.usePreDockPose + " useManualSpeed " + PickupObjectMessage.useManualSpeed);

    QueueSingleAction.action.pickupObject = PickupObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.kLocalBusyTime;

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void RollObject(ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    RollObjectMessage.objectID = selectedObject;
    RollObjectMessage.usePreDockPose = usePreDockPose;
    RollObjectMessage.useManualSpeed = useManualSpeed;
    RollObjectMessage.motionProf = PathMotionProfileDefault;

    DAS.Debug(this, "Roll Object " + RollObjectMessage.objectID + " usePreDockPose " + RollObjectMessage.usePreDockPose + " useManualSpeed " + RollObjectMessage.useManualSpeed);
    
    QueueSingleAction.action.rollObject = RollObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.kLocalBusyTime;

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    PlaceObjectOnGroundMessage.x_mm = position.x;
    PlaceObjectOnGroundMessage.y_mm = position.y;
    PlaceObjectOnGroundMessage.qx = rotation.x;
    PlaceObjectOnGroundMessage.qy = rotation.y;
    PlaceObjectOnGroundMessage.qz = rotation.z;
    PlaceObjectOnGroundMessage.qw = rotation.w;
    PlaceObjectOnGroundMessage.level = System.Convert.ToByte(level);
    PlaceObjectOnGroundMessage.useManualSpeed = useManualSpeed;
    PlaceObjectOnGroundMessage.motionProf = PathMotionProfileDefault;
    
    DAS.Debug(this, "Drop Object At Pose " + position + " useManualSpeed " + useManualSpeed);

    QueueSingleAction.action.placeObjectOnGround = PlaceObjectOnGroundMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();
    
    LocalBusyTimer = CozmoUtil.kLocalBusyTime;

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void GotoPose(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    float rotationAngle = rotation.eulerAngles.z * Mathf.Deg2Rad;
    GotoPose(position.x, position.y, rotationAngle, level, useManualSpeed, callback, queueActionPosition);
  }

  public void GotoPose(float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    GotoPoseMessage.level = System.Convert.ToByte(level);
    GotoPoseMessage.useManualSpeed = useManualSpeed;
    GotoPoseMessage.x_mm = x_mm;
    GotoPoseMessage.y_mm = y_mm;
    GotoPoseMessage.rad = rad;
    GotoPoseMessage.motionProf = PathMotionProfileDefault;

    DAS.Debug(this, "Go to Pose: x: " + GotoPoseMessage.x_mm + " y: " + GotoPoseMessage.y_mm + " useManualSpeed: " + GotoPoseMessage.useManualSpeed + " level: " + GotoPoseMessage.level);

    QueueSingleAction.action.goToPose = GotoPoseMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();
    
    LocalBusyTimer = CozmoUtil.kLocalBusyTime;

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void GotoObject(ObservedObject obj, float distance_mm, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    GotoObjectMessage.objectID = obj;
    GotoObjectMessage.distanceFromObjectOrigin_mm = distance_mm;
    GotoObjectMessage.useManualSpeed = false;
    GotoObjectMessage.motionProf = PathMotionProfileDefault;

    QueueSingleAction.action.goToObject = GotoObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();
    
    LocalBusyTimer = CozmoUtil.kLocalBusyTime;
    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void AlignWithObject(ObservedObject obj, float distanceFromMarker_mm, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    AlignWithObjectMessage.objectID = obj;
    AlignWithObjectMessage.distanceFromMarker_mm = distanceFromMarker_mm;
    AlignWithObjectMessage.useManualSpeed = false;
    AlignWithObjectMessage.useApproachAngle = false;
    AlignWithObjectMessage.motionProf = PathMotionProfileDefault;

    QueueSingleAction.action.alignWithObject = AlignWithObjectMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.kLocalBusyTime;
    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  // Height factor should be between 0.0f and 1.0f
  // 0.0f being lowest and 1.0f being highest.
  public void SetLiftHeight(float heightFactor, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "SetLiftHeight: " + heightFactor);
    if (LiftHeightFactor == heightFactor && queueActionPosition == QueueActionPosition.NOW) {
      if (callback != null) {
        callback(true);
      }
      return;
    }

    SetLiftHeightMessage.accel_rad_per_sec2 = 5f;
    SetLiftHeightMessage.max_speed_rad_per_sec = 10f;
    SetLiftHeightMessage.height_mm = (heightFactor * (CozmoUtil.kMaxLiftHeightMM - CozmoUtil.kMinLiftHeightMM)) + CozmoUtil.kMinLiftHeightMM;

    QueueSingleAction.action.setLiftHeight = SetLiftHeightMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;

    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void SetRobotCarryingObject(int objectID = -1) {
    DAS.Debug(this, "Set Robot Carrying Object: " + objectID);
    
    SetRobotCarryingObjectMessage.robotID = ID;
    SetRobotCarryingObjectMessage.objectID = objectID;

    RobotEngineManager.Instance.Message.SetRobotCarryingObject = SetRobotCarryingObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    TargetLockedObject = null;
    
    SetLiftHeight(0f);
    SetHeadAngle();
  }

  public void ClearAllBlocks() {
    DAS.Debug(this, "Clear All Blocks");
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

  public void TurnInPlace(float angle_rad, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    TurnInPlaceMessage.robotID = ID;
    TurnInPlaceMessage.angle_rad = angle_rad;

    QueueSingleAction.action.turnInPlace = TurnInPlaceMessage;
    var tag = GetNextIdTag();
    QueueSingleAction.idTag = tag;
    QueueSingleAction.position = queueActionPosition;
    QueueSingleAction.inSlot = 0;
    RobotEngineManager.Instance.Message.QueueSingleAction = QueueSingleAction;
    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }

  public void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false) {
    DAS.Debug(this, "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose);

    TraverseObjectMessage.useManualSpeed = useManualSpeed;
    TraverseObjectMessage.usePreDockPose = usePreDockPose;

    RobotEngineManager.Instance.Message.TraverseObject = TraverseObjectMessage;
    RobotEngineManager.Instance.SendMessage();

    LocalBusyTimer = CozmoUtil.kLocalBusyTime;
  }

  public void SetVisionMode(VisionMode mode, bool enable) {
    EnableVisionModeMessage.mode = mode;
    EnableVisionModeMessage.enable = enable;

    RobotEngineManager.Instance.Message.EnableVisionMode = EnableVisionModeMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void ExecuteBehavior(BehaviorType type) {
    ExecuteBehaviorMessage.behaviorType = type;
    
    DAS.Debug(this, "Execute Behavior " + ExecuteBehaviorMessage.behaviorType);

    RobotEngineManager.Instance.Message.ExecuteBehavior = ExecuteBehaviorMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetBehaviorSystem(bool enable) {
    SetBehaviorSystemEnabledMessage.enabled = enable;

    RobotEngineManager.Instance.Message.SetBehaviorSystemEnabled = SetBehaviorSystemEnabledMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  public void ActivateBehaviorChooser(BehaviorChooserType behaviorChooserType) {
    DAS.Debug(this, "ActivateBehaviorChooser: " + behaviorChooserType);

    ActivateBehaviorChooserMessage.behaviorChooserType = behaviorChooserType;

    RobotEngineManager.Instance.Message.ActivateBehaviorChooser = ActivateBehaviorChooserMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  #region Light manipulation

  #region Backpack Lights

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
      uint colorUint = CozmoPalette.ColorToUInt(color);
      SetBackpackLED((int)ledToChange, colorUint);
    }
  }

  public void SetBackbackArrowLED(LEDId ledId, float intensity) {
    intensity = Mathf.Clamp(intensity, 0f, 1f);
    Color color = new Color(intensity, intensity, intensity);
    uint colorUint = CozmoPalette.ColorToUInt(color);
    SetBackpackLED((int)ledId, colorUint);
  }

  public void SetFlashingBackpackLED(LEDId ledToChange, Color color, uint onDurationMs = 200, uint offDurationMs = 200, uint transitionDurationMs = 0) {
    uint colorUint = CozmoPalette.ColorToUInt(color);
    SetBackpackLED((int)ledToChange, colorUint, 0, onDurationMs, offDurationMs, transitionDurationMs, transitionDurationMs);
  }

  private void SetBackpackLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                               uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {
    for (int i = 0; i < BackpackLights.Length; ++i) {
      SetBackpackLED(i, onColor, offColor, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms);
    }
  }

  private void SetBackpackLED(int index, uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                              uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {
    // Special case for arrow lights; they only accept red as a color
    if (index == (int)LEDId.LED_BACKPACK_LEFT || index == (int)LEDId.LED_BACKPACK_RIGHT) {
      //uint whiteUint = CozmoPalette.ColorToUInt(Color.white);
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

  // should only be called from update loop
  private void SetAllBackpackLEDs() {

    SetBackpackLEDsMessage.robotID = ID;
    for (int i = 0; i < BackpackLights.Length; i++) {
      SetBackpackLEDsMessage.onColor[i] = BackpackLights[i].OnColor;
      SetBackpackLEDsMessage.offColor[i] = BackpackLights[i].OffColor;
      SetBackpackLEDsMessage.onPeriod_ms[i] = BackpackLights[i].OnPeriodMs;
      SetBackpackLEDsMessage.offPeriod_ms[i] = BackpackLights[i].OffPeriodMs;
      SetBackpackLEDsMessage.transitionOnPeriod_ms[i] = BackpackLights[i].TransitionOnPeriodMs;
      SetBackpackLEDsMessage.transitionOffPeriod_ms[i] = BackpackLights[i].TransitionOffPeriodMs;
    }

    RobotEngineManager.Instance.Message.SetBackpackLEDs = SetBackpackLEDsMessage;
    RobotEngineManager.Instance.SendMessage();

    SetLastLEDs();
  }

  private void SetLastLEDs() {
    for (int i = 0; i < BackpackLights.Length; ++i) {
      BackpackLights[i].SetLastInfo();
    }
  }

  #endregion

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

  #endregion
}
