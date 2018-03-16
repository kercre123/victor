using Cozmo.BlockPool;
using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Cozmo.Util;
using G2U = Anki.Cozmo.ExternalInterface;


/// <summary>
///   our unity side representation of cozmo's current state
///   also wraps most messages related solely to him
/// </summary>
public class Robot : IRobot {

  public class Light : ILight {
    private uint _LastOnColor;

    public uint OnColor { get; set; }

    private uint _LastOffColor;

    public uint OffColor { get; set; }

    private uint _LastOnPeriodMs;

    public uint OnPeriodMs { get; set; }

    private uint _LastOffPeriodMs;

    public uint OffPeriodMs { get; set; }

    private uint _LastTransitionOnPeriodMs;

    public uint TransitionOnPeriodMs { get; set; }

    private uint _LastTransitionOffPeriodMs;

    public uint TransitionOffPeriodMs { get; set; }

    public int _LastOffset;

    public int Offset { get; set; }

    public void SetLastInfo() {
      _LastOnColor = OnColor;
      _LastOffColor = OffColor;
      _LastOnPeriodMs = OnPeriodMs;
      _LastOffPeriodMs = OffPeriodMs;
      _LastTransitionOnPeriodMs = TransitionOnPeriodMs;
      _LastTransitionOffPeriodMs = TransitionOffPeriodMs;
      _LastOffset = Offset;
    }

    public bool Changed {
      get {
        return _LastOnColor != OnColor || _LastOffColor != OffColor || _LastOnPeriodMs != OnPeriodMs || _LastOffPeriodMs != OffPeriodMs ||
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
      Offset = 0;

      _LastOnColor = 0;
      _LastOffColor = 0;
      _LastOnPeriodMs = 1000;
      _LastOffPeriodMs = 0;
      _LastTransitionOnPeriodMs = 0;
      _LastTransitionOffPeriodMs = 0;
      _LastOffset = 0;

      MessageDelay = 0f;
    }

    public static float MessageDelay = 0f;

    public const uint FOREVER = uint.MaxValue;
  }

  private struct RobotCallbackWrapper {
    public readonly uint IdTag;

    private readonly RobotCallback _Callback;

    public RobotCallbackWrapper(uint idTag, RobotCallback callback) {
      IdTag = idTag;
      _Callback = callback;
    }

    public bool IsCallback(RobotCallback callback) {
      return _Callback == callback;
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

  #region consts

  public const float kDefaultRadPerSec = 4.3f;
  public const float kPanAccel_radPerSec2 = 10f;
  public const float kPanTolerance_rad = 5 * Mathf.Deg2Rad;
  private const float kDefaultResetBackupSpeed = 30.0f;

  #endregion

  public byte ID { get; private set; }

  // in radians
  public float HeadAngle { get; private set; }

  // robot yaw in radians, from negative PI to positive PI
  public float PoseAngle { get; private set; }

  public float PitchAngle { get; private set; }

  public float RollAngle { get; private set; }
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
  public OffTreadsState TreadState { get; private set; }
  public bool HasHiccups { get; private set; }

  public GameStatusFlag GameStatus { get; private set; }

  public float BatteryVoltage { get; private set; }

  // objects that are currently visible (cubes, charger)
  public Dictionary<int, ObservableObject> VisibleObjects { get; private set; }

  // objects with poses known by blockworld
  public Dictionary<int, ObservableObject> KnownObjects { get; private set; }

  // cubes that are active and we can talk to / hear
  public Dictionary<int, LightCube> LightCubes { get; private set; }

  public event LightCubeStateEventHandler OnLightCubeAdded;
  public event LightCubeStateEventHandler OnLightCubeRemoved;

  public Dictionary<int, LightCube> VisibleLightCubes { get; private set; }

  public event ChargerStateEventHandler OnChargerAdded;
  public event ChargerStateEventHandler OnChargerRemoved;

  private static AllTriggersConsidered _AllTriggers = new AllTriggersConsidered(true, true, true, true, true, true,
                                                                       true, true, true, true, true, true,
                                                                       true, true, true, true, true, true,
                                                                       true, true, true);

  private static DisableReactionsWithLock _RequestDisableReactions = new DisableReactionsWithLock("unity", _AllTriggers);
  private static RemoveDisableReactionsLock _RequestReEnableReactions = new RemoveDisableReactionsLock("unity");

  private ActiveObject _Charger = null;

  public ActiveObject Charger {
    get { return _Charger; }
    private set {
      if (value != null) {
        _Charger = value;
        if (OnChargerAdded != null) {
          OnChargerAdded(_Charger);
        }
      }
      else {
        ActiveObject oldCharger = _Charger;
        _Charger = null;
        if (oldCharger != null && OnChargerRemoved != null) {
          OnChargerRemoved(oldCharger);
        }
      }
    }
  }

  public List<Face> Faces { get; private set; }

  public event FaceStateEventHandler OnFaceAdded;
  public event FaceStateEventHandler OnFaceRemoved;
  public event EnrolledFaceRemoved OnEnrolledFaceRemoved;
  public event EnrolledFaceRenamed OnEnrolledFaceRenamed;

  public Dictionary<int, string> EnrolledFaces { get; set; }

  public Dictionary<int, float> EnrolledFacesLastEnrolledTime { get; set; }

  public Dictionary<int, float> EnrolledFacesLastSeenTime { get; set; }

  public event PetFaceStateEventHandler OnPetFaceAdded;
  public event PetFaceStateEventHandler OnPetFaceRemoved;

  public List<PetFace> PetFaces { get; private set; }

  public int FriendshipPoints { get; private set; }

  public int FriendshipLevel { get; private set; }

  public float[] EmotionValues { get; private set; }

  public ILight[] BackpackLights { get; private set; }

  public bool IsSparked { get; private set; }

  public Anki.Cozmo.UnlockId SparkUnlockId { get; private set; }

  private bool _LightsChanged {
    get {
      for (int i = 0; i < BackpackLights.Length; ++i) {
        if (BackpackLights[i].Changed)
          return true;
      }

      return false;
    }
  }

  private ActiveObject _targetLockedObject = null;

  public ActiveObject TargetLockedObject {
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

  public BehaviorClass CurrentBehaviorClass { get; set; }
  public ReactionTrigger CurrentReactionTrigger { get; set; }

  public BehaviorID CurrentBehaviorID { get; set; }

  public string CurrentDebugAnimationString { get; set; }

  public uint FirmwareVersion { get; set; }

  public uint SerialNumber { get; set; }

  public BodyColor BodyColor { get; set; }

  public int BodyHWVersion { get; set; }

  private PathMotionProfile PathMotionProfileDefault;

  private uint _NextIdTag = (uint)Anki.Cozmo.ActionConstants.FIRST_GAME_TAG;

  private ObservableObject _CarryingObject = null;

  public ObservableObject CarryingObject {
    get {
      if (_CarryingObject == null || _CarryingObject.ID != CarryingObjectID)
        _CarryingObject = GetActiveObjectById(CarryingObjectID);

      return _CarryingObject;
    }
  }

  public event Action<ObservableObject> OnCarryingObjectSet;
  public event Action<int> OnNumBlocksConnectedChanged;
  public event Action<FaceEnrollmentCompleted> OnEnrolledFaceComplete;

  private ObservableObject _HeadTrackingObject = null;

  public ObservableObject HeadTrackingObject {
    get {
      if (_HeadTrackingObject == null || _HeadTrackingObject.ID != HeadTrackingObjectID)
        _HeadTrackingObject = GetActiveObjectById(HeadTrackingObjectID);

      return _HeadTrackingObject;
    }
  }

  public event Action<ObservableObject> OnHeadTrackingObjectSet;

  private readonly List<RobotCallbackWrapper> _RobotCallbacks = new List<RobotCallbackWrapper>();

  public bool Status(RobotStatusFlag s) {
    return (RobotStatus & s) == s;
  }

  public bool IsLocalized() {
    return (GameStatus & GameStatusFlag.IsLocalized) == GameStatusFlag.IsLocalized;
  }

  private uint _LastProcessedVisionFrameEngineTimestamp;

  public Robot(byte robotID) : base() {
    ID = robotID;

    VisibleObjects = new Dictionary<int, ObservableObject>();
    KnownObjects = new Dictionary<int, ObservableObject>();

    LightCubes = new Dictionary<int, LightCube>();
    VisibleLightCubes = new Dictionary<int, LightCube>();

    Faces = new List<global::Face>();
    EnrolledFaces = new Dictionary<int, string>();
    EnrolledFacesLastEnrolledTime = new Dictionary<int, float>();
    EnrolledFacesLastSeenTime = new Dictionary<int, float>();
    PetFaces = new List<PetFace>();

    // Defaults in clad
    PathMotionProfileDefault = new PathMotionProfile();

    BackpackLights = new ILight[Singleton<SetBackpackLEDs>.Instance.onColor.Length];

    EmotionValues = new float[(int)Anki.Cozmo.EmotionType.Count];

    for (int i = 0; i < BackpackLights.Length; ++i) {
      BackpackLights[i] = new Light();
    }

    ClearData(true);

    RobotEngineManager.Instance.BlockPoolTracker.OnBlockDataConnectionChanged += HandleBlockDataConnectionChanged;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Reset);

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(ProcessRobotCompletedAction);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.MoodState>(UpdateEmotionFromEngineRobotManager);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(SparkEnded);

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ObjectTapped>(HandleActiveObjectTapped);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotProcessedImage>(FinishedProcessingImage);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDeletedLocatedObject>(HandleDeleteObservedObject);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotObservedObject>(HandleSeeObservedObject);
    RobotEngineManager.Instance.AddCallback<ObjectMoved>(HandleActiveObjectMoved);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.LocatedObjectStates>(HandleLocatedObjectStatesUpdate);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ConnectedObjectStates>(HandleConnectedObjectStatesUpdate);
    RobotEngineManager.Instance.AddCallback<ObjectStoppedMoving>(HandleObservedObjectStoppedMoving);
    RobotEngineManager.Instance.AddCallback<ObjectUpAxisChanged>(HandleObservedObjectUpAxisChanged);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotMarkedObjectPoseUnknown>(HandleObservedObjectPoseUnknown);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDeletedFace>(HandleDeletedFace);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DebugAnimationString>(HandleDebugAnimationString);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotState>(UpdateInfo);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotOffTreadsStateChanged>(HandleRobotOffTreadsStateChanged);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DebugString>(HandleDebugString);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotObservedFace>(UpdateObservedFaceInfo);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotErasedEnrolledFace>(HandleRobotErasedEnrolledFace);
    RobotEngineManager.Instance.AddCallback<Anki.Vision.RobotRenamedEnrolledFace>(HandleRobotRenamedEnrolledFace);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.BehaviorTransition>(HandleBehaviorTransition);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotObservedPet>(UpdateObservedPetFaceInfo);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted>(HandleEnrolledFace);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotHiccupsChanged>(HandleRobotHiccupsChanged);
    RobotEngineManager.Instance.AddCallback<ReactionTriggerTransition>(HandleReactionTriggerTransition);

    ObservableObject.AnyInFieldOfViewStateChanged += HandleInFieldOfViewStateChanged;
    RobotEngineManager.Instance.AddCallback<Anki.Vision.LoadedKnownFace>(HandleLoadedKnownFace);

    if ((RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim)) {
      DisableReactionsWithLock("sim", ReactionaryBehaviorEnableGroups.kSimulatedRobotTriggers);
    }
  }

  public void Dispose() {
    RobotEngineManager.Instance.BlockPoolTracker.OnBlockDataConnectionChanged -= HandleBlockDataConnectionChanged;

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Reset);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(ProcessRobotCompletedAction);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(SparkEnded);

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ObjectTapped>(HandleActiveObjectTapped);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotProcessedImage>(FinishedProcessingImage);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDeletedLocatedObject>(HandleDeleteObservedObject);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotObservedObject>(HandleSeeObservedObject);
    RobotEngineManager.Instance.RemoveCallback<ObjectMoved>(HandleActiveObjectMoved);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.LocatedObjectStates>(HandleLocatedObjectStatesUpdate);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ConnectedObjectStates>(HandleConnectedObjectStatesUpdate);
    RobotEngineManager.Instance.RemoveCallback<ObjectStoppedMoving>(HandleObservedObjectStoppedMoving);
    RobotEngineManager.Instance.RemoveCallback<ObjectUpAxisChanged>(HandleObservedObjectUpAxisChanged);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotMarkedObjectPoseUnknown>(HandleObservedObjectPoseUnknown);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDeletedFace>(HandleDeletedFace);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DebugAnimationString>(HandleDebugAnimationString);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotState>(UpdateInfo);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DebugString>(HandleDebugString);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotObservedFace>(UpdateObservedFaceInfo);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotErasedEnrolledFace>(HandleRobotErasedEnrolledFace);
    RobotEngineManager.Instance.RemoveCallback<Anki.Vision.RobotRenamedEnrolledFace>(HandleRobotRenamedEnrolledFace);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.BehaviorTransition>(HandleBehaviorTransition);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotObservedPet>(UpdateObservedPetFaceInfo);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted>(HandleEnrolledFace);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotHiccupsChanged>(HandleRobotHiccupsChanged);
    RobotEngineManager.Instance.RemoveCallback<ReactionTriggerTransition>(HandleReactionTriggerTransition);

    ActiveObject.AnyInFieldOfViewStateChanged -= HandleInFieldOfViewStateChanged;
    RobotEngineManager.Instance.RemoveCallback<Anki.Vision.LoadedKnownFace>(HandleLoadedKnownFace);

    if ((RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim)) {
      RemoveDisableReactionsLock("sim");
    }
  }

  public Vector3 WorldToCozmo(Vector3 worldSpacePosition) {
    Vector3 offset = worldSpacePosition - this.WorldPosition;
    offset = Quaternion.Inverse(this.Rotation) * offset;
    return offset;
  }

  private void HandleRobotErasedEnrolledFace(Anki.Cozmo.ExternalInterface.RobotErasedEnrolledFace message) {
    EnrolledFaces.Remove(message.faceID);
    EnrolledFacesLastEnrolledTime.Remove(message.faceID);
    EnrolledFacesLastSeenTime.Remove(message.faceID);
    if (OnEnrolledFaceRemoved != null) {
      OnEnrolledFaceRemoved(message.faceID, message.name);
    }
  }

  private void HandleDebugString(Anki.Cozmo.ExternalInterface.DebugString message) {
    CurrentBehaviorString = message.text;
  }

  private void HandleBehaviorTransition(Anki.Cozmo.ExternalInterface.BehaviorTransition message) {
    CurrentBehaviorClass = message.newBehaviorClass;
    CurrentBehaviorID = message.newBehaviorID;
  }

  private void HandleReactionTriggerTransition(ReactionTriggerTransition message) {
    CurrentReactionTrigger = message.newTrigger;
  }

  private void HandleDebugAnimationString(Anki.Cozmo.ExternalInterface.DebugAnimationString message) {
    CurrentDebugAnimationString = message.text;
  }

  private void ProcessRobotCompletedAction(Anki.Cozmo.ExternalInterface.RobotCompletedAction message) {

    uint idTag = message.idTag;
    RobotActionType actionType = (RobotActionType)message.actionType;
    bool success = message.result == ActionResult.SUCCESS;

    DAS.Info("Robot.ActionCallback", "Type = " + actionType + " success = " + success);

    for (int i = 0; i < _RobotCallbacks.Count; ++i) {
      if (_RobotCallbacks[i].IdTag == idTag) {
        var callback = _RobotCallbacks[i];
        _RobotCallbacks.RemoveAt(i);
        callback.Invoke(success);
        break;
      }
    }
  }

  private void HandleEnrolledFace(FaceEnrollmentCompleted message) {
    if (message.result == Anki.Cozmo.FaceEnrollmentResult.Success) {
      if (EnrolledFaces.ContainsKey(message.faceID)) {
        DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Re-enrolled existing face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(message.name));
        EnrolledFaces[message.faceID] = message.name;
        EnrolledFacesLastEnrolledTime[message.faceID] = Time.time;
        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnReEnrollFace);
      }
      else {
        EnrolledFaces.Add(message.faceID, message.name);
        EnrolledFacesLastEnrolledTime.Add(message.faceID, 0);
        EnrolledFacesLastSeenTime.Add(message.faceID, 0);
        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnMeetNewPerson);
        DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Enrolled new face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(message.name));

        // log using up another face slot to das
        DAS.Event("robot.face_slots_used", EnrolledFaces.Count.ToString(),
          DASUtil.FormatExtraData("1"));
      }
    }

    if (OnEnrolledFaceComplete != null) {
      OnEnrolledFaceComplete(message);
    }
  }

  private uint GetNextIdTag() {
    // Post increment _NextIdTag (and loop within the GAME_TAG range)
    uint nextIdTag = _NextIdTag;
    if (_NextIdTag == (uint)Anki.Cozmo.ActionConstants.LAST_GAME_TAG) {
      _NextIdTag = (uint)Anki.Cozmo.ActionConstants.FIRST_GAME_TAG;
    }
    else {
      ++_NextIdTag;
    }

    return nextIdTag;
  }

  private int SortByDistance(ActiveObject a, ActiveObject b) {
    float d1 = ((Vector2)a.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
    float d2 = ((Vector2)b.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
    return d1.CompareTo(d2);
  }

  private void Reset(object message) {
    ClearData();
  }

  private void HandleLoadedKnownFace(Anki.Vision.LoadedKnownFace loadedKnownFaceMessage) {

    DAS.Info("Robot.HandleLoadedKnownFace.NumFace", "EnrolledFaces.Count: " + EnrolledFaces.Count + " LoadedKnownFace Id: " + loadedKnownFaceMessage.faceID);
    foreach (KeyValuePair<int, string> kvp in EnrolledFaces) {
      DAS.Info("Robot.HandleLoadedKnownFace.EnrolledFace", "Enrolled face: " + kvp.Key + " " + PrivacyGuard.HidePersonallyIdentifiableInfo(kvp.Value));
    }

    if (EnrolledFaces.ContainsKey(loadedKnownFaceMessage.faceID)) {
      DAS.Error("Robot.HandleLoadedKnownFace", "Attempting to load already existing face ID: " + loadedKnownFaceMessage.faceID);
      return;
    }
    EnrolledFaces.Add(loadedKnownFaceMessage.faceID, loadedKnownFaceMessage.name);
    // TODO: hook up secondsSinceFirstEnrolled when concept of "how long Cozmo has known this person is needed
    EnrolledFacesLastEnrolledTime.Add(loadedKnownFaceMessage.faceID, Time.time - loadedKnownFaceMessage.secondsSinceLastUpdated);
    EnrolledFacesLastSeenTime.Add(loadedKnownFaceMessage.faceID, Time.time - loadedKnownFaceMessage.secondsSinceLastSeen);
  }

  private void HandleRobotRenamedEnrolledFace(Anki.Vision.RobotRenamedEnrolledFace message) {
    EnrolledFaces[message.faceID] = message.name;
    if (OnEnrolledFaceRenamed != null) {
      OnEnrolledFaceRenamed(message.faceID, message.name);
    }
  }

  public bool IsLightCubeInPickupRange(LightCube lightCube) {
    var bounds = new Bounds(
                   new Vector3(CozmoUtil.kOriginToLowLiftDDistMM, 0, CozmoUtil.kBlockLengthMM * 0.5f),
                   Vector3.one * CozmoUtil.kBlockLengthMM);

    return bounds.Contains(WorldToCozmo(lightCube.WorldPosition));
  }

  public void ResetRobotState(Action onComplete = null, bool disableFreeplay = true) {
    DriveWheels(0.0f, 0.0f);
    TrackToObject(null);
    CancelAllCallbacks();
    RobotStartIdle(disableFreeplay);
    SetNightVision(false);

    SetBackpackLEDs(Color.black.ToUInt());
    SetAllBackpackLEDs();

    TryResetHeadAndLift(onComplete);
  }

  public void RobotStartIdle(bool disableFreeplay = true) {
    if (disableFreeplay) {
      SetEnableFreeplayActivity(false);
    }
    Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = { };
    float[] paramValues = { };
    SetLiveIdleAnimationParameters(paramNames, paramValues, true);
    TurnTowardsLastFacePose(Mathf.PI);
  }

  public void RobotResumeFromIdle(bool freePlay) {
    SetEnableFreeplayActivity(freePlay);
    // TODO: any additional functionality that we want for Cozmo resuming from a
    // Cozmo Hold Context Switch
  }

  public void TryResetHeadAndLift(Action onComplete) {
    foreach (var lightCube in LightCubes.Values) {
      if (IsLightCubeInPickupRange(lightCube)) {
        // there is a light cube in the way, so we need to backup first
        DriveStraightAction(kDefaultResetBackupSpeed, -(CozmoUtil.kOriginToLowLiftDDistMM), callback: (success) => {
          TryResetHeadAndLift(onComplete);
        });
        return;
      }
    }

    ResetLiftAndHead((s) => {
      DAS.Debug("Robot.TryResetHeadAndLift", "Reset Lift And Head Complete! " + s);
      if (s == false) {
        DAS.Warn("Robot.TryResetHeadAndLift", "Reset Lift and Head failed!");
      }
      if (onComplete != null) {
        onComplete();
      }
    });
  }

  public void ClearData(bool initializing = false) {
    if (!initializing) {
      TurnOffAllLights(true);
      DAS.Debug(this, "Robot data cleared");
    }

    LightCubes.Clear();
    TreadState = OffTreadsState.OnTreads; // default from engine.
    RobotStatus = RobotStatusFlag.NoneRobotStatusFlag;
    GameStatus = GameStatusFlag.Nothing;
    WorldPosition = Vector3.zero;
    Rotation = Quaternion.identity;
    CarryingObjectID = -1;
    HeadTrackingObjectID = -1;
    _LastHeadTrackingObjectID = -1;
    _CarryingObject = null;
    _HeadTrackingObject = null;
    HeadAngle = float.MaxValue;
    PoseAngle = float.MaxValue;
    LeftWheelSpeed = float.MaxValue;
    RightWheelSpeed = float.MaxValue;
    LiftHeight = float.MaxValue;
    BatteryVoltage = float.MaxValue;
    _LastProcessedVisionFrameEngineTimestamp = 0;
    CurrentBehaviorClass = BehaviorClass.Wait;
    CurrentReactionTrigger = ReactionTrigger.NoneTrigger;
    // usually this is unique
    CurrentBehaviorID = BehaviorID.Wait;
    HasHiccups = false;

    for (int i = 0; i < BackpackLights.Length; ++i) {
      BackpackLights[i].ClearData();
    }

  }

  private void UpdateInfo(G2U.RobotState message) {
    HeadAngle = message.headAngle_rad;
    PoseAngle = message.poseAngle_rad;
    PitchAngle = message.posePitch_rad;
    RollAngle = (float)Math.Atan2(message.accel.y, message.accel.z);
    LeftWheelSpeed = message.leftWheelSpeed_mmps;
    RightWheelSpeed = message.rightWheelSpeed_mmps;
    LiftHeight = message.liftHeight_mm;
    RobotStatus = (RobotStatusFlag)message.status;
    GameStatus = (GameStatusFlag)message.gameStatus;
    BatteryVoltage = message.batteryVoltage;
    CarryingObjectID = message.carryingObjectID;
    HeadTrackingObjectID = message.headTrackingObjectID;

    WorldPosition = new Vector3(message.pose.x, message.pose.y, message.pose.z);
    Rotation = new Quaternion(message.pose.q1, message.pose.q2, message.pose.q3, message.pose.q0);
  }

  private void HandleRobotOffTreadsStateChanged(G2U.RobotOffTreadsStateChanged message) {
    TreadState = message.treadsState;
  }

  private void HandleRobotHiccupsChanged(G2U.RobotHiccupsChanged message) {
    HasHiccups = message.hasHiccups;
  }

  public LightCube GetLightCubeWithFactoryID(uint factoryID) {
    foreach (LightCube lc in LightCubes.Values) {
      if (lc.FactoryID == factoryID) {
        return lc;
      }
    }

    return null;
  }

  public LightCube GetLightCubeWithObjectType(ObjectType objectType) {
    foreach (LightCube lc in LightCubes.Values) {
      if (lc.ObjectType == objectType) {
        return lc;
      }
    }

    return null;
  }

  public uint SendQueueSingleAction<T>(T action, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, byte numRetries = 0) {
    var tag = GetNextIdTag();
    RobotEngineManager.Instance.Message.QueueSingleAction =
      Singleton<QueueSingleAction>.Instance.Initialize(
      idTag: tag,
      numRetries: numRetries,
      position: queueActionPosition,
      actionState: action);
    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
    return tag;
  }

  public void SendQueueCompoundAction(Anki.Cozmo.ExternalInterface.RobotActionUnion[] actions, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool isParallel = false) {
    var tag = GetNextIdTag();

    for (int i = 0; i < actions.Length; ++i) {
      for (int j = i + 1; j < actions.Length; ++j) {
        if (System.Object.ReferenceEquals(actions[i].GetState(), actions[j].GetState())) {
          DAS.Error("UnityRobot.SendQueueCompondAction", "Matching action with same reference address. You shouldn't use Singleton<> to queue compond actions of the same type");
        }
      }
    }

    RobotEngineManager.Instance.Message.QueueCompoundAction =
      Singleton<QueueCompoundAction>.Instance.Initialize(
      idTag: tag,
      numRetries: 0,
      parallel: isParallel,
      position: queueActionPosition,
      actions: actions);
    RobotEngineManager.Instance.SendMessage();

    _RobotCallbacks.Add(new RobotCallbackWrapper(tag, callback));
  }


  #region Mood Stats

  public void VisualizeQuad(Vector3 lowerLeft, Vector3 upperRight) {

    RobotEngineManager.Instance.Message.VisualizeQuad =
      Singleton<VisualizeQuad>.Instance.Initialize(
      quadID: 0,
      color: Color.black.ToUInt(),
      xUpperLeft: lowerLeft.x,
      yUpperLeft: upperRight.y,
      // not sure how to play z. Assume its constant between lower left and upper right?
      zUpperLeft: lowerLeft.z,

      xLowerLeft: lowerLeft.x,
      yLowerLeft: lowerLeft.y,
      zLowerLeft: lowerLeft.z,

      xUpperRight: upperRight.x,
      yUpperRight: upperRight.y,
      zUpperRight: upperRight.z,

      xLowerRight: upperRight.x,
      yLowerRight: lowerLeft.y,
      zLowerRight: upperRight.z
    );
    RobotEngineManager.Instance.SendMessage();
  }

  // source is a identifier for where the emotion is being set so it can penalize the
  // amount affected if it's coming too often from the same source.
  public void AddToEmotion(Anki.Cozmo.EmotionType type, float deltaValue, string source) {

    RobotEngineManager.Instance.Message.MoodMessage =
      Singleton<MoodMessage>.Instance.Initialize(
      Singleton<AddToEmotion>.Instance.Initialize(type, deltaValue, source));
    RobotEngineManager.Instance.SendMessage();
  }

  // Only debug panels should be using set.
  // You should not be calling this from a challenge.
  public void SetEmotion(Anki.Cozmo.EmotionType type, float value) {
    RobotEngineManager.Instance.Message.MoodMessage =
      Singleton<MoodMessage>.Instance.Initialize(
      Singleton<SetEmotion>.Instance.Initialize(type, value));
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetCalibrationData(float focalLengthX, float focalLengthY, float centerX, float centerY) {
    float[] dummyDistortionCoeffs = { 0, 0, 0, 0, 0, 0, 0, 0 };
    RobotEngineManager.Instance.Message.CameraCalibration = Singleton<CameraCalibration>.Instance.Initialize(focalLengthX, focalLengthY, centerX, centerY, 0.0f, 240, 320, dummyDistortionCoeffs);
    RobotEngineManager.Instance.SendMessage();
  }

  private void UpdateEmotionFromEngineRobotManager(Anki.Cozmo.ExternalInterface.MoodState moodStateMessage) {
    for (EmotionType i = 0; i < EmotionType.Count; ++i) {
      EmotionValues[(int)i] = moodStateMessage.emotionValues[(int)i];
    }

  }

  #endregion

  private void DeleteActiveObject(int id) {
    if (id < 0) {
      // Negative object ids are invalid
      return;
    }

    bool removedObject = false;

    if (Charger != null && Charger.ID == id) {
      removedObject = true;
      Charger = null;
    }
    else {
      LightCube removedLightCube;
      LightCubes.TryGetValue(id, out removedLightCube);
      removedObject = LightCubes.Remove(id);
      if (removedObject) {
        DAS.Debug("Robot.DeleteObservedObject", "Deleted ID " + id);
        if (OnLightCubeRemoved != null) {
          OnLightCubeRemoved(removedLightCube);
        }
      }
      else {
        DAS.Debug("Robot.DeleteObservedObject", "Tried to delete object with ID " + id + " but failed.");
      }
    }
  }

  public void HandleDeleteObservedObject(Anki.Cozmo.ExternalInterface.RobotDeletedLocatedObject message) {
    KnownObjects.Remove((int)message.objectID);
    ActiveObject objectPoseUnknown = GetActiveObjectById((int)message.objectID);
    if (objectPoseUnknown != null) {
      objectPoseUnknown.MarkPoseUnknown();
    }
  }

  public void FinishedProcessingImage(Anki.Cozmo.ExternalInterface.RobotProcessedImage message) {
    uint engineTimestamp = message.timestamp;
    // Update the robot's current timestamp if the new one is newer
    if (engineTimestamp >= _LastProcessedVisionFrameEngineTimestamp) {
      _LastProcessedVisionFrameEngineTimestamp = engineTimestamp;

      bool isMarkersFrame = false;
      bool isFaceFrame = false;
      bool isPetFrame = false;
      for (int i = 0; i < message.visionModes.Length; i++) {
        switch (message.visionModes[i]) {
        case VisionMode.DetectingMarkers:
          isMarkersFrame = true;
          break;
        case VisionMode.DetectingFaces:
          isFaceFrame = true;
          break;
        case VisionMode.DetectingPets:
          isPetFrame = true;
          break;
        default:
          break;
        }
      }

      if (isMarkersFrame) {
        // Iterate through cubes / charger and increase not seen frame count of cube if 
        // cube's last seen timestamp is outdated / less than robot's current timestamp
        if (Charger != null && Charger.LastSeenEngineTimestamp < engineTimestamp) {
          Charger.MarkNotVisibleThisFrame();
        }
        foreach (var kvp in LightCubes) {
          if (kvp.Value.LastSeenEngineTimestamp < engineTimestamp) {
            kvp.Value.MarkNotVisibleThisFrame();
          }
        }
        foreach (var kvp in KnownObjects) {
          if (kvp.Value.LastSeenEngineTimestamp < engineTimestamp) {
            kvp.Value.MarkNotVisibleThisFrame();
          }
        }
      }

      if (isFaceFrame) {
        foreach (var face in Faces) {
          if (face.LastSeenEngineTimestamp < engineTimestamp) {
            face.MarkNotVisibleThisFrame();
          }
        }
      }

      if (isPetFrame) {
        List<PetFace> petFacesToRemove = new List<PetFace>();
        foreach (var petFace in PetFaces) {
          if (petFace.LastSeenEngineTimestamp < engineTimestamp) {
            petFace.MarkNotVisibleThisFrame();
            if (!petFace.IsInFieldOfView) {
              petFacesToRemove.Add(petFace);
            }
          }
        }

        foreach (var petFaceToRemove in petFacesToRemove) {
          PetFaces.Remove(petFaceToRemove);
          if (OnPetFaceRemoved != null) {
            OnPetFaceRemoved(petFaceToRemove);
          }
        }
      }
    }
    else {
      DAS.Error("Robot.FinishedProcessingImage.OldTimestamp", "Received old RobotProcessedImage message with timestamp " + engineTimestamp
      + " _after_ receiving a newer message with timestamp " + _LastProcessedVisionFrameEngineTimestamp + "!");

      // Reset back to zero in case we got a gigantic timestamp which would prevent any future (good) images from getting processed, 
      // even if they have reasonable timestamps
      _LastProcessedVisionFrameEngineTimestamp = 0;
    }
  }

  private void HandleBlockDataConnectionChanged(BlockPoolData blockData) {
    //    DAS.Debug("Robot.HandleObjectConnectionState", (blockData.IsConnected ? "Connected " : "Disconnected ")
    //              + "object of type " + blockData.ObjectType + " with ID " + blockData.ObjectID
    //              + " and factoryId " + blockData.FactoryID.ToString("X"));

    if (blockData.ConnectionState == BlockConnectionState.Unavailable || blockData.ConnectionState == BlockConnectionState.Connected) {
      if (blockData.IsConnected) {
        AddActiveObject((int)blockData.ObjectID, blockData.FactoryID, blockData.ObjectType);
      }
      else {
        DeleteActiveObject((int)blockData.ObjectID);
      }
    }

    if (OnNumBlocksConnectedChanged != null) {
      OnNumBlocksConnectedChanged(LightCubes.Count);
    }
  }

  private void HandleDeletedFace(Anki.Cozmo.ExternalInterface.RobotDeletedFace message) {
    int index = -1;
    for (int i = 0; i < Faces.Count; i++) {
      if (Faces[i].ID == message.faceID) {
        index = i;
        break;
      }
    }

    if (index != -1) {
      Face removedFace = Faces[index];
      Faces.RemoveAt(index);
      if (OnFaceRemoved != null) {
        OnFaceRemoved(removedFace);
      }
    }
  }

  private void HandleChangedObservedFaceID(Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID message) {
    for (int i = 0; i < Faces.Count; i++) {
      // remove the old face we saw, new face will be added by RobotObservedFace
      if (Faces[i].ID == message.oldID) {
        if (OnFaceRemoved != null) {
          OnFaceRemoved(Faces[i]);
        }
        Faces.RemoveAt(i);
        break;
      }
    }
  }

  private void HandleLocatedObjectStatesUpdate(G2U.LocatedObjectStates message) {
    // first flag all objects as Unknown because LocatedObjectStates only broadcasts objects that currently
    // exist in the current coordinate frame. Flag all Unknown, and then Update the poses of those received
    MarkAllActiveObjectsPoseAsUnknown();

    LocatedObjectState[] locObjectStates = message.objects;
    if (locObjectStates != null) {
      for (int i = 0; i < locObjectStates.Length; ++i) {
        LocatedObjectState obj = locObjectStates[i];

        ObservableObject knownObj = null;
        if (KnownObjects.TryGetValue((int)obj.objectID, out knownObj)) {
          knownObj.UpdateAvailable(obj);
        }

        ActiveObject objectSeen = GetActiveObjectById((int)obj.objectID);
        if (objectSeen != null) {
          objectSeen.UpdateAvailable(obj);
        }
      }
    }
  }

  private void HandleConnectedObjectStatesUpdate(G2U.ConnectedObjectStates message) {
    // Do we care about this in Unity?
  }

  private void HandleSeeObservedObject(Anki.Cozmo.ExternalInterface.RobotObservedObject message) {

    DasTracker.Instance.TrackObjectObserved(message);

    if (message.objectFamily == Anki.Cozmo.ObjectFamily.Mat) {
      DAS.Warn("Robot.HandleSeeObservedObject", "HandleSeeObservedObject received RobotObservedObject message about the Mat!");
      return;
    }

    ObservableObject knownObj = null;
    if ((KnownObjects.TryGetValue((int)message.objectID, out knownObj))) {
      knownObj.UpdateInfo(message);
    }

    ActiveObject objectSeen = GetActiveObjectById((int)message.objectID);
    // if we have an active object with the same id then update its pose
    if (objectSeen != null) {
      // Update its info if the new timestamp is newer or the same as the robot's current timestamp
      if (message.timestamp >= objectSeen.LastSeenEngineTimestamp) {
        objectSeen.UpdateInfo(message);
      }
      else {
        DAS.Error("Robot.HandleSeeObservedObject", "Received old RobotObservedObject message with timestamp " + message.timestamp
        + " _after_ receiving a newer message with timestamp " + objectSeen.LastSeenEngineTimestamp + "!");
      }
    }

  }

  private void HandleActiveObjectMoved(ObjectMoved message) {
    ActiveObject objectStartedMoving = GetActiveObjectById((int)message.objectID);
    if (objectStartedMoving != null) {
      // Mark pose dirty and is moving if the new timestamp is newer or the same as the robot's current timestamp
      if (message.timestamp >= objectStartedMoving.LastMovementMessageEngineTimestamp) {
        objectStartedMoving.HandleStartedMoving(message);
      }
      else {
        DAS.Error("Robot.HandleActiveObjectMoved", "Received old ObjectMoved message with timestamp " + message.timestamp
        + " _after_ receiving a newer message with timestamp " + objectStartedMoving.LastMovementMessageEngineTimestamp + "!");
      }
    }
  }

  private void HandleObservedObjectStoppedMoving(ObjectStoppedMoving message) {
    ActiveObject objectStoppedMoving = GetActiveObjectById((int)message.objectID);
    if (objectStoppedMoving != null) {
      // Mark is moving false if the new timestamp is newer or the same as the robot's current timestamp
      if (message.timestamp >= objectStoppedMoving.LastMovementMessageEngineTimestamp) {
        objectStoppedMoving.HandleStoppedMoving(message);
      }
      else {
        DAS.Error("Robot.HandleObservedObjectStoppedMoving", "Received old ObjectStoppedMoving message with timestamp " + message.timestamp
        + " _after_ receiving a newer message with timestamp " + objectStoppedMoving.LastMovementMessageEngineTimestamp + "!");
      }
    }
  }


  private void HandleObservedObjectUpAxisChanged(ObjectUpAxisChanged message) {
    ActiveObject objectUpAxisChanged = GetActiveObjectById((int)message.objectID);
    if (objectUpAxisChanged != null) {
      // Mark is moving false if the new timestamp is newer or the same as the robot's current timestamp
      if (message.timestamp >= objectUpAxisChanged.LastUpAxisChangedMessageEngineTimestamp) {
        objectUpAxisChanged.HandleUpAxisChanged(message);
      }
      else {
        DAS.Error("Robot.HandleObservedObjectUpAxisChanged", "Received old ObjectUpAxis message with timestamp " + message.timestamp
        + " _after_ receiving a newer message with timestamp " + objectUpAxisChanged.LastUpAxisChangedMessageEngineTimestamp + "!");
      }
    }
  }


  private void HandleObservedObjectPoseUnknown(Anki.Cozmo.ExternalInterface.RobotMarkedObjectPoseUnknown message) {
    int objectID = (int)message.objectID;
    KnownObjects.Remove((int)message.objectID);

    ActiveObject objectPoseUnknown = GetActiveObjectById(objectID);
    if (objectPoseUnknown != null) {
      objectPoseUnknown.MarkPoseUnknown();
    }
  }

  private void HandleActiveObjectTapped(ObjectTapped message) {
    ActiveObject objectPoseUnknown = GetActiveObjectById((int)message.objectID);
    if (objectPoseUnknown != null) {
      objectPoseUnknown.HandleObjectTapped(message);
    }
  }

  // Flags the pose of all active objects as Unknown
  public void MarkAllActiveObjectsPoseAsUnknown() {
    if (Charger != null) {
      Charger.MarkPoseUnknown();
    }
    foreach (var kvp in LightCubes) {
      kvp.Value.MarkPoseUnknown();
    }
  }

  public ActiveObject GetActiveObjectById(int id) {
    ActiveObject foundObject = null;
    if (Charger != null && Charger.ID == id) {
      foundObject = Charger;
    }
    else {
      LightCube foundCube;
      LightCubes.TryGetValue(id, out foundCube);
      foundObject = foundCube;
    }

    return foundObject;
  }

  public ActiveObject GetActiveObjectWithFactoryID(uint factoryId) {
    ActiveObject foundObject = null;
    if (Charger != null && Charger.FactoryID == factoryId) {
      foundObject = Charger;
    }
    else {
      foreach (var kvp in LightCubes) {
        if (kvp.Value.FactoryID == factoryId) {
          foundObject = kvp.Value;
        }
      }
    }

    return foundObject;
  }

  private ActiveObject AddActiveObject(int id, uint factoryId, ObjectType objectType) {
    if (id < 0) {
      // Negative object ids are invalid
      return null;
    }

    ActiveObject createdObject = null;
    bool isCube = ((objectType == ObjectType.Block_LIGHTCUBE1)
                  || (objectType == ObjectType.Block_LIGHTCUBE2)
                  || (objectType == ObjectType.Block_LIGHTCUBE3));
    if (isCube) {
      LightCube lightCube = null;
      if (!LightCubes.TryGetValue(id, out lightCube)) {
        DAS.Debug("Robot.AddActiveObject", "Registered LightCube: id=" + id + " factoryId = " + factoryId.ToString("X") + " objectType=" + objectType);
        lightCube = new LightCube(id, factoryId, ObjectFamily.LightCube, objectType);

        LightCubes.Add(id, lightCube);
        if (OnLightCubeAdded != null) {
          OnLightCubeAdded(lightCube);
        }
      }
      else {
        if ((lightCube.FactoryID == 0) && (factoryId != 0)) {
          // So far we received messages about the cube without a factory ID. This is because it was seen 
          // before we connected to it. This message has the factory ID so we need to update its information
          // so we can find it in the future
          DAS.Debug("Robot.AddActiveObject", "Updating cube " + id + " factoryID to " + factoryId.ToString("X"));
          lightCube.FactoryID = factoryId;
        }
      }
      createdObject = lightCube;
    }
    else if (objectType == ObjectType.Charger_Basic) {
      if (Charger == null) {
        DAS.Debug("Robot.AddActiveObject", "Registered Charger: id=" + id + " factoryId = " + factoryId.ToString("X") + " objectType=" + objectType);
        Charger = new ActiveObject(id, factoryId, ObjectFamily.Charger, objectType);
        createdObject = Charger;
      }
    }
    else if ((objectType == ObjectType.ProxObstacle)
             || (objectType == ObjectType.CollisionObstacle)
             || (objectType == ObjectType.CliffDetection)) {
      // Just ignore cliff/obstacle observations because game/Unity doesn't care about them
      return null;
    }
    else {
      DAS.Warn("Robot.AddActiveObject", "Tried to add an object with unsupported ObjectType! id=" + id + " objectType=" + objectType);
    }

    return createdObject;
  }

  private void UpdateObservedFaceInfo(Anki.Cozmo.ExternalInterface.RobotObservedFace message) {
    Face face = Faces.Find(x => x.ID == message.faceID);
    // TODO add image face info
    AddObservedFace(face != null ? face : null, message);
    if (EnrolledFacesLastSeenTime.ContainsKey(message.faceID)) {
      EnrolledFacesLastSeenTime[message.faceID] = Time.time;
    }
  }

  private void UpdateObservedPetFaceInfo(Anki.Cozmo.ExternalInterface.RobotObservedPet message) {
    PetFace petFace = PetFaces.Find(x => x.PetID == message.petID);
    if (petFace == null) {
      petFace = new PetFace(message);
      PetFaces.Add(petFace);
      if (OnPetFaceAdded != null) {
        OnPetFaceAdded(petFace);
      }
    }
    else {
      petFace.UpdateInfo(message);
    }
  }

  public void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams) {

    RobotEngineManager.Instance.Message.DisplayProceduralFace =
      Singleton<DisplayProceduralFace>.Instance.Initialize(
      faceAngle_deg: faceAngle,
      faceCenX: faceCenter.x,
      faceCenY: faceCenter.y,
      faceScaleX: faceScale.x,
      faceScaleY: faceScale.y,
      leftEye: leftEyeParams,
      rightEye: rightEyeParams,
      duration_ms: 1000
    );
    RobotEngineManager.Instance.SendMessage();

  }

  private void AddObservedFace(Face faceObject, G2U.RobotObservedFace message) {
    if (faceObject == null) {
      faceObject = new Face(message);
      Faces.Add(faceObject);
      if (OnFaceAdded != null) {
        OnFaceAdded(faceObject);
      }
    }
    else {
      faceObject.UpdateInfo(message);
    }
  }

  public void DriveHead(float speed_radps) {
    RobotEngineManager.Instance.Message.MoveHead = Singleton<MoveHead>.Instance.Initialize(speed_radps);
    RobotEngineManager.Instance.SendMessage();
  }

  public void MoveLift(float speed_radps) {
    RobotEngineManager.Instance.Message.MoveLift = Singleton<MoveLift>.Instance.Initialize(speed_radps);
    RobotEngineManager.Instance.SendMessage();
  }

  public void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps) {
    RobotEngineManager.Instance.Message.DriveWheels =
      Singleton<DriveWheels>.Instance.Initialize(leftWheelSpeedMmps, rightWheelSpeedMmps, 0, 0);
    RobotEngineManager.Instance.SendMessage();
  }

  /// <summary>
  /// Drives the motors so that cozmo moves in an arc.
  /// Set curveRadiusMm to 32768 to go straight.
  /// When curveRadiusMm is negative, he turns to his right; positive = left
  /// Set curveRadiusMm to 0 for a point turn. wheelSpeedMmps must then be in rad/s.
  /// Alternatively, you can set curveRadiusMm to 1 or -1 for something close enough to a point turn where wheelSpeedMmps remains in mm/s.
  /// </summary>
  public void DriveArc(float wheelSpeedMmps, short curveRadiusMm, float accelMmps = 0.0f) {
    RobotEngineManager.Instance.Message.DriveArc =
                        Singleton<DriveArc>.Instance.Initialize(wheelSpeedMmps, accelMmps, curveRadiusMm);
    RobotEngineManager.Instance.SendMessage();
  }

  public void StopAllMotors() {
    RobotEngineManager.Instance.Message.StopAllMotors = Singleton<StopAllMotors>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public uint PlaceObjectOnGroundHere(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "Place Object " + CarryingObject + " On Ground Here");

    return SendQueueSingleAction(Singleton<PlaceObjectOnGroundHere>.Instance, callback, queueActionPosition);

  }

  public uint PlaceObjectRel(ObservableObject target, float offsetFromMarker, float approachAngle, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "PlaceObjectRel " + target.ID);

    return SendQueueSingleAction(Singleton<PlaceRelObject>.Instance.Initialize(
      approachAngle_rad: approachAngle,
      placementOffsetX_mm: offsetFromMarker,
      objectID: target.ID,
      useApproachAngle: true,
      usePreDockPose: true,
      useManualSpeed: false,
      motionProf: PathMotionProfileDefault),
      callback,
      queueActionPosition);
  }

  public uint PlaceOnObject(ObservableObject target, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "PlaceOnObject " + target.ID);

    return SendQueueSingleAction(Singleton<PlaceOnObject>.Instance.Initialize(
      objectID: target.ID,
      usePreDockPose: true,
      useApproachAngle: false,
      approachAngle_rad: 0f,
      useManualSpeed: false,
      checkForObjectOnTop: checkForObjectOnTop,
      motionProf: PathMotionProfileDefault
    ),
      callback,
      queueActionPosition);
  }

  public void CancelAction(RobotActionType actionType = RobotActionType.UNKNOWN) {
    DAS.Debug(this, "CancelAction actionType(" + actionType + ")");

    RobotEngineManager.Instance.Message.CancelAction = Singleton<CancelAction>.Instance.Initialize(actionType);
    RobotEngineManager.Instance.SendMessage();
  }

  public void CancelActionByIdTag(uint idTag) {
    DAS.Debug(this, "CancelActionByIdTag (" + idTag + ")");

    RobotEngineManager.Instance.Message.CancelActionByIdTag = Singleton<CancelActionByIdTag>.Instance.Initialize(idTag);
    RobotEngineManager.Instance.SendMessage();
  }

  public void CancelCallback(RobotCallback callback) {
    for (int i = _RobotCallbacks.Count - 1; i >= 0; i--) {
      if (_RobotCallbacks[i].IsCallback(callback)) {
        _RobotCallbacks.RemoveAt(i);
      }
    }
  }

  public void CancelAllCallbacks() {
    _RobotCallbacks.Clear();
  }

  public void SetFaceToEnroll(int existingID, string name, bool saveToRobot = true, bool sayName = true, bool useMusic = true) {

    DAS.Debug(this, "Sending SetFaceToEnroll for name=" + PrivacyGuard.HidePersonallyIdentifiableInfo(name)
    + " to be saved as existing ID=" + existingID);

    RobotEngineManager.Instance.Message.SetFaceToEnroll = Singleton<SetFaceToEnroll>.Instance.Initialize(name, 0, existingID, saveToRobot, sayName, useMusic);
    RobotEngineManager.Instance.SendMessage();
  }

  public void CancelFaceEnrollment() {
    RobotEngineManager.Instance.Message.CancelFaceEnrollment = Singleton<CancelFaceEnrollment>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public uint SendAnimationTrigger(AnimationTrigger animTriggerEvent, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, bool useSafeLiftMotion = true, bool ignoreBodyTrack = false, bool ignoreHeadTrack = false, bool ignoreLiftTrack = false, uint loops = 1) {

    DAS.Debug(this, "Sending Trigger " + animTriggerEvent + " with " + loops + " loop " + useSafeLiftMotion);
    return SendQueueSingleAction(Singleton<PlayAnimationTrigger>.Instance.Initialize(loops, animTriggerEvent, useSafeLiftMotion, ignoreBodyTrack, ignoreHeadTrack, ignoreLiftTrack), callback, queueActionPosition);
  }

  public void SetLiveIdleAnimationParameters(Anki.Cozmo.LiveIdleAnimationParameter[] paramNames, float[] paramValues,
                                             bool setUnspecifiedToDefault = false) {
    RobotEngineManager.Instance.Message.SetLiveIdleAnimationParameters =
      Singleton<SetLiveIdleAnimationParameters>.Instance.Initialize(paramNames, paramValues, setUnspecifiedToDefault);
    RobotEngineManager.Instance.SendMessage();
  }

  public uint PlayNeedsGetOutAnimIfNeeded(RobotCallback callback = null) {
    return SendQueueSingleAction(Singleton<PlayNeedsGetOutAnimIfNeeded>.Instance, callback);
  }

  public void PushDrivingAnimations(AnimationTrigger drivingStartAnim, AnimationTrigger drivingLoopAnim, AnimationTrigger drivingEndAnim, string lockName) {
    RobotEngineManager.Instance.Message.PushDrivingAnimations =
      Singleton<PushDrivingAnimations>.Instance.Initialize(drivingStartAnim, drivingLoopAnim, drivingEndAnim, lockName);
    RobotEngineManager.Instance.SendMessage();
  }

  public void RemoveDrivingAnimations(string lockName) {
    RobotEngineManager.Instance.Message.RemoveDrivingAnimations = Singleton<RemoveDrivingAnimations>.Instance.Initialize(lockName);
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
  public uint SetHeadAngle(float angleFactor = -0.8f,
                           RobotCallback callback = null,
                           QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                           bool useExactAngle = false, float speed_radPerSec = -1, float accel_radPerSec2 = -1) {

    float radians = CozmoUtil.HeadAngleFactorToRadians(angleFactor, useExactAngle);

    if (HeadTrackingObject == -1 && Mathf.Abs(radians - HeadAngle) < 0.001f && queueActionPosition == QueueActionPosition.NOW) {
      if (callback != null) {
        callback(true);
      }
      return (uint)ActionConstants.INVALID_TAG;
    }

    if (speed_radPerSec <= 0f) {
      speed_radPerSec = CozmoUtil.kMoveLiftSpeed_radPerSec;
    }

    if (accel_radPerSec2 <= 0f) {
      accel_radPerSec2 = CozmoUtil.kMoveLiftAccel_radPerSec2;
    }

    return SendQueueSingleAction(
      Singleton<SetHeadAngle>.Instance.Initialize(
        radians,
        max_speed_rad_per_sec: speed_radPerSec,
        accel_rad_per_sec2: accel_radPerSec2,
        duration_sec: 0.0f),
      callback,
      queueActionPosition);
  }

  /// <summary>
  /// Tells engine to preserve a head and lift state even if reactions are triggered
  /// This should be called with enabled as false to cancel the defaults before leaving the current activity
  /// </summary>
  /// <param name="headAngle">headAngle</param> usually from -1 (MIN_HEAD_ANGLE) to 1 (kMaxHeadAngle)
  /// <param name="liftHeight">liftHeight</param> the height to set the lift to
  public void SetDefaultHeadAndLiftState(bool enable, float headAngleFactor, float liftHeight) {

    float radians = CozmoUtil.HeadAngleFactorToRadians(headAngleFactor, true);

    RobotEngineManager.Instance.Message.SetDefaultHeadAndLiftState =
    Singleton<SetDefaultHeadAndLiftState>.Instance.Initialize(
      enable,
      radians,
      liftHeight
    );

    RobotEngineManager.Instance.SendMessage();

  }

  public void SetRobotVolume(float volume) {
    DAS.Debug(this, "Set Robot Volume " + volume);

    RobotEngineManager.Instance.Message.SetRobotVolume = Singleton<SetRobotVolume>.Instance.Initialize(ID, volume);
    RobotEngineManager.Instance.SendMessage();
  }

  public float GetRobotVolume() {
    float volume = 1f;
    Dictionary<Anki.Cozmo.Audio.VolumeParameters.VolumeType, float> currentVolumePrefs = DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.VolumePreferences;
    currentVolumePrefs.TryGetValue(Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot, out volume);
    return volume;
  }

  public void TrackToObject(ObservableObject observableObject, bool headOnly = true) {
    if (HeadTrackingObjectID == observableObject && _LastHeadTrackingObjectID == observableObject) {
      return;
    }

    _LastHeadTrackingObjectID = observableObject;

    uint objId;
    if (observableObject != null) {
      objId = (uint)observableObject;
    }
    else {
      objId = uint.MaxValue; //cancels tracking
    }

    DAS.Debug(this, "Track Head To Object " + objId);

    RobotEngineManager.Instance.Message.TrackToObject =
      Singleton<TrackToObject>.Instance.Initialize(objId, headOnly);
    RobotEngineManager.Instance.SendMessage();

  }

  public ActiveObject GetCharger() {
    return Charger;
  }

  public uint MountCharger(ActiveObject charger, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SendQueueSingleAction(
      Singleton<MountCharger>.Instance.Initialize(
        charger.ID, PathMotionProfileDefault, false, false),
      callback, queueActionPosition
    );
  }

  public void StopTrackToObject() {
    TrackToObject(null);
  }

  /// <summary>
  /// Turns towards the object and then adjusts the head. Set maxTurnAngle_rad to 0 to only tilt the head.
  /// </summary>
  public uint TurnTowardsObject(ObservableObject observedObject, bool headTrackWhenDone = true, float maxPanSpeed_radPerSec = kDefaultRadPerSec, float panAccel_radPerSec2 = kPanAccel_radPerSec2,
                                RobotCallback callback = null,
                                QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                                float setTiltTolerance_rad = 0f, float maxTurnAngle_rad = float.MaxValue) {

    DAS.Debug(this, "Face Object " + observedObject);

    return SendQueueSingleAction(Singleton<TurnTowardsObject>.Instance.Initialize(
      objectID: observedObject,
      maxTurnAngle_rad: maxTurnAngle_rad,
      panTolerance_rad: kPanTolerance_rad, // 1.7 degrees is the minimum in the engine
      headTrackWhenDone: headTrackWhenDone,
      maxPanSpeed_radPerSec: maxPanSpeed_radPerSec,
      panAccel_radPerSec2: panAccel_radPerSec2,
      maxTiltSpeed_radPerSec: 0f,
      tiltAccel_radPerSec2: 0f,
      tiltTolerance_rad: setTiltTolerance_rad,
      visuallyVerifyWhenDone: false
    ),
      callback,
      queueActionPosition);

  }

  public uint TurnTowardsFace(Face face, float maxTurnAngle_rad = Mathf.PI, float maxPanSpeed_radPerSec = kDefaultRadPerSec, float panAccel_radPerSec2 = kPanAccel_radPerSec2,
                              bool sayName = false, AnimationTrigger namedTrigger = AnimationTrigger.Count,
                              AnimationTrigger unnamedTrigger = AnimationTrigger.Count,
                              RobotCallback callback = null,
                              QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    return SendQueueSingleAction(
      Singleton<Anki.Cozmo.ExternalInterface.TurnTowardsFace>.Instance.Initialize(
        faceID: face.ID,
        maxTurnAngle_rad: maxTurnAngle_rad,
        maxPanSpeed_radPerSec: maxPanSpeed_radPerSec,
        panAccel_radPerSec2: panAccel_radPerSec2,
        panTolerance_rad: kPanTolerance_rad, // 1.7 degrees is the minimum in the engine
        maxTiltSpeed_radPerSec: 0f,
        tiltAccel_radPerSec2: 0f,
        tiltTolerance_rad: 0f,
        sayName: sayName,
        namedTrigger: namedTrigger,
        unnamedTrigger: unnamedTrigger
      ),
      callback,
      queueActionPosition);
  }

  // Turns towards the last seen face, but not any more than the specified maxTurnAngle
  public uint TurnTowardsLastFacePose(float maxTurnAngle, bool sayName = false,
                                      AnimationTrigger namedTrigger = AnimationTrigger.Count,
                                      AnimationTrigger unnamedTrigger = AnimationTrigger.Count,
                                      RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    DAS.Debug(this, "TurnTowardsLastFacePose with maxTurnAngle : " + maxTurnAngle);

    return SendQueueSingleAction(Singleton<TurnTowardsLastFacePose>.Instance.Initialize(
      maxTurnAngle_rad: maxTurnAngle,
      maxPanSpeed_radPerSec: kDefaultRadPerSec,
      panAccel_radPerSec2: kPanAccel_radPerSec2,
      panTolerance_rad: kPanTolerance_rad,
      maxTiltSpeed_radPerSec: 0f,
      tiltAccel_radPerSec2: 0f,
      tiltTolerance_rad: 0f,
      sayName: sayName,
      namedTrigger: namedTrigger,
      unnamedTrigger: unnamedTrigger
    ),
      callback,
      queueActionPosition);
  }

  public uint PickupObject(ObservableObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool useApproachAngle = false, float approachAngleRad = 0.0f, bool checkForObjectOnTop = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    DAS.Debug(this, "Pick And Place Object " + selectedObject + " usePreDockPose " + usePreDockPose + " useManualSpeed " + useManualSpeed);

    return SendQueueSingleAction(
      Singleton<PickupObject>.Instance.Initialize(
        objectID: selectedObject,
        motionProf: PathMotionProfileDefault,
        approachAngle_rad: approachAngleRad,
        useApproachAngle: useApproachAngle,
        useManualSpeed: useManualSpeed,
        usePreDockPose: usePreDockPose,
        checkForObjectOnTop: checkForObjectOnTop
      ),
      callback,
      queueActionPosition);
  }

  public uint RollObject(ObservableObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false, bool checkForObjectOnTop = true, bool rollWithoutDocking = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    DAS.Debug(this, "Roll Object " + selectedObject + " usePreDockPose " + usePreDockPose + " useManualSpeed " + usePreDockPose);

    return SendQueueSingleAction(Singleton<RollObject>.Instance.Initialize(
      selectedObject,
      PathMotionProfileDefault,
      doDeepRoll: false,
      approachAngle_rad: 0f,
      useApproachAngle: false,
      usePreDockPose: usePreDockPose,
      useManualSpeed: useManualSpeed,
      checkForObjectOnTop: checkForObjectOnTop,
      rollWithoutDocking: rollWithoutDocking
    ),
      callback,
      queueActionPosition);
  }

  public uint PlaceObjectOnGround(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    DAS.Debug(this, "Drop Object At Pose " + position + " useManualSpeed " + useManualSpeed);


    return SendQueueSingleAction(Singleton<PlaceObjectOnGround>.Instance.Initialize(
      x_mm: position.x,
      y_mm: position.y,
      qw: rotation.w,
      qx: rotation.x,
      qy: rotation.y,
      qz: rotation.z,
      motionProf: PathMotionProfileDefault,
      level: System.Convert.ToByte(level),
      useManualSpeed: useManualSpeed,
      useExactRotation: false,
      checkDestinationFree: false
    ),
      callback,
      queueActionPosition);
  }

  public uint GotoPose(Vector3 position, Quaternion rotation, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    float rotationAngle = rotation.eulerAngles.z * Mathf.Deg2Rad;
    return GotoPose(position.x, position.y, rotationAngle, level, useManualSpeed, callback, queueActionPosition);
  }

  public uint GotoPose(float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "Go to Pose: x: " + x_mm + " y: " + y_mm + " rad: " + rad);

    return SendQueueSingleAction(Singleton<GotoPose>.Instance.Initialize(
      level: System.Convert.ToByte(level),
      useManualSpeed: useManualSpeed,
      x_mm: x_mm,
      y_mm: y_mm,
      rad: rad,
      motionProf: PathMotionProfileDefault
    ),
      callback,
      queueActionPosition);
  }

  public uint DriveStraightAction(float speed_mmps, float dist_mm, bool shouldPlayAnimation = true, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SendQueueSingleAction(Singleton<DriveStraight>.Instance.Initialize(
      speed_mmps,
      dist_mm,
      shouldPlayAnimation
    ),
      callback,
      queueActionPosition);
  }

  public uint GotoObject(ObservableObject obj, float distance_mm, bool goToPreDockPose = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SendQueueSingleAction(Singleton<GotoObject>.Instance.Initialize(
      objectID: obj,
      distanceFromObjectOrigin_mm: distance_mm,
      useManualSpeed: false,
      usePreDockPose: goToPreDockPose,
      motionProf: PathMotionProfileDefault
    ),
      callback,
      queueActionPosition);
  }

  public uint AlignWithObject(ObservableObject obj, float distanceFromMarker_mm, RobotCallback callback = null, bool useApproachAngle = false, bool usePreDockPose = false, float approachAngleRad = 0.0f, AlignmentType alignmentType = AlignmentType.CUSTOM, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, byte numRetries = 0) {
    return SendQueueSingleAction(
      Singleton<AlignWithObject>.Instance.Initialize(
        objectID: obj,
        motionProf: PathMotionProfileDefault,
        distanceFromMarker_mm: distanceFromMarker_mm,
        approachAngle_rad: approachAngleRad,
        alignmentType: alignmentType,
        useApproachAngle: useApproachAngle,
        usePreDockPose: usePreDockPose,
        useManualSpeed: false
      ),
      callback,
      queueActionPosition,
      numRetries);
  }

  public LightCube GetClosestLightCube() {
    float minDist = float.MaxValue;
    ActiveObject closest = null;
    foreach (var kvp in LightCubes) {
      if (kvp.Value.CurrentPoseState == ActiveObject.PoseState.Known) {
        float d = Vector3.Distance(kvp.Value.WorldPosition, WorldPosition);
        if (d < minDist) {
          minDist = d;
          closest = kvp.Value;
        }
      }
    }
    return closest as LightCube;
  }

  public uint SearchForCube(int cubeID, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SearchForNearbyObject(cubeID, callback, queueActionPosition);
  }

  // If an objectID is passed in, the action will complete successfully as soon as the object is seen
  // otherwise, cozmo will complete a full look around nearby before completing
  public uint SearchForNearbyObject(int objectId = -1, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                                    float backupDistance_mm = (float)SearchForNearbyObjectDefaults.BackupDistance_mm,
                                    float backupSpeed_mm = (float)SearchForNearbyObjectDefaults.BackupSpeed_mms,
                                    float headAngle_rad = Mathf.Deg2Rad * (float)SearchForNearbyObjectDefaults.HeadAngle_deg) {
    return SendQueueSingleAction(
      Singleton<SearchForNearbyObject>.Instance.Initialize(
        desiredObjectID: objectId,
        backupDistance_mm: backupDistance_mm,
        backupSpeed_mms: backupSpeed_mm,
        headAngle_rad: headAngle_rad
      ),
      callback,
      queueActionPosition);
  }

  public void ResetLiftAndHead(RobotCallback callback = null) {
    RobotActionUnion[] actions = {
      new RobotActionUnion().Initialize(Singleton<SetLiftHeight>.Instance.Initialize(
        height_mm: 0.0f,
        max_speed_rad_per_sec: CozmoUtil.kMoveLiftSpeed_radPerSec,
        accel_rad_per_sec2: CozmoUtil.kMoveLiftAccel_radPerSec2,
        duration_sec: 0f)),
      new RobotActionUnion().Initialize(Singleton<SetHeadAngle>.Instance.Initialize(
        angle_rad: CozmoUtil.HeadAngleFactorToRadians(0.25f, false),
        max_speed_rad_per_sec: CozmoUtil.kMoveHeadSpeed_radPerSec,
        accel_rad_per_sec2: CozmoUtil.kMoveHeadAccel_radPerSec2,
        duration_sec: 0.0f))
    };

    SendQueueCompoundAction(actions, callback);
  }

  // Height factor should be between 0.0f and 1.0f
  // 0.0f being lowest and 1.0f being highest.
  public uint SetLiftHeight(float heightFactor, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW, float speed_radPerSec = -1, float accel_radPerSec2 = -1) {
    DAS.Debug(this, "SetLiftHeight: " + heightFactor);

    if (speed_radPerSec <= 0f) {
      speed_radPerSec = CozmoUtil.kMoveLiftSpeed_radPerSec;
    }

    if (accel_radPerSec2 <= 0f) {
      accel_radPerSec2 = CozmoUtil.kMoveLiftAccel_radPerSec2;
    }

    return SendQueueSingleAction(Singleton<SetLiftHeight>.Instance.Initialize(
      height_mm: (heightFactor * (CozmoUtil.kMaxLiftHeightMM - CozmoUtil.kMinLiftHeightMM)) + CozmoUtil.kMinLiftHeightMM,
      max_speed_rad_per_sec: speed_radPerSec,
      accel_rad_per_sec2: accel_radPerSec2,
      duration_sec: 0f
    ),
      callback,
      queueActionPosition);
  }

  public void SetRobotCarryingObject(int objectID = -1) {
    DAS.Debug(this, "Set Robot Carrying Object: " + objectID);

    RobotEngineManager.Instance.Message.SetRobotCarryingObject =
      Singleton<SetRobotCarryingObject>.Instance.Initialize(objectID);
    RobotEngineManager.Instance.SendMessage();

    SetLiftHeight(0f);
    SetHeadAngle();
  }

  // rsam: meaning has changed. Probably no need to support now
  //  public void ClearAllBlocks() {
  //    DAS.Debug(this, "Clear All Blocks");
  //    RobotEngineManager.Instance.Message.ClearAllBlocks = Singleton<ClearAllBlocks>.Instance.Initialize(ID);
  //    RobotEngineManager.Instance.SendMessage();
  //    Reset(null);
  //
  //    SetLiftHeight(0f);
  //    SetHeadAngle();
  //  }
  //
  //  public void ClearAllObjects() {
  //    RobotEngineManager.Instance.Message.ClearAllObjects = Singleton<ClearAllObjects>.Instance.Initialize(ID);
  //    RobotEngineManager.Instance.SendMessage();
  //    Reset(null);
  //  }

  public void VisionWhileMoving(bool enable) {
    RobotEngineManager.Instance.Message.VisionWhileMoving = Singleton<VisionWhileMoving>.Instance.Initialize(System.Convert.ToByte(enable));
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetEnableCliffSensor(bool enabled) {
    RobotEngineManager.Instance.Message.EnableCliffSensor = Singleton<EnableCliffSensor>.Instance.Initialize(enabled);
    RobotEngineManager.Instance.SendMessage();
  }

  public void RequestRandomGame() {
    RobotEngineManager.Instance.Message.BehaviorManagerMessage =
      Singleton<BehaviorManagerMessage>.Instance.Initialize(
      Singleton<PlayAGameRequest>.Instance
      );
    RobotEngineManager.Instance.SendMessage();
  }

  public void DoRandomSpark() {
    RobotEngineManager.Instance.Message.BehaviorManagerMessage =
      Singleton<BehaviorManagerMessage>.Instance.Initialize(
      Singleton<DoATrickRequest>.Instance
      );
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetCurrentSpark(UnlockId id) {
    IsSparked = (id != UnlockId.Count);
    SparkUnlockId = id;
  }

  public void EnableSparkUnlock(Anki.Cozmo.UnlockId id) {
    SetCurrentSpark(id);

    RobotEngineManager.Instance.Message.BehaviorManagerMessage =
      Singleton<BehaviorManagerMessage>.Instance.Initialize(
      Singleton<ActivateSpark>.Instance.Initialize(SparkUnlockId)
    );
    RobotEngineManager.Instance.SendMessage();
  }

  public void StopSparkUnlock() {
    EnableSparkUnlock(UnlockId.Count);
  }

  private void SparkEnded(object message) {
    IsSparked = false;
    SparkUnlockId = UnlockId.Count;
  }

  public void SetSparkedMusicState(Anki.AudioMetaData.SwitchState.Sparked sparkedState) {
    RobotEngineManager.Instance.Message.SetSparkedMusicState = Singleton<SetSparkedMusicState>.Instance.Initialize(sparkedState);
    RobotEngineManager.Instance.SendMessage();
  }

  public uint TurnInPlace(float angle_rad, float speed_rad_per_sec, float accel_rad_per_sec2, float tolerance_rad = 0.0f, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {

    return SendQueueSingleAction(Singleton<TurnInPlace>.Instance.Initialize(
      angle_rad,
      speed_rad_per_sec,
      accel_rad_per_sec2,
      tolerance_rad, // Tolerance of 0.0f means use default tolerance in engine which is defined by POINT_TURN_ANGLE_TOL in cozmoEngineConfig.h
      0
    ),
      callback,
      queueActionPosition);
  }

  public void TraverseObject(int objectID, bool usePreDockPose = false, bool useManualSpeed = false) {
    DAS.Debug(this, "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose);

    RobotEngineManager.Instance.Message.TraverseObject =
      Singleton<TraverseObject>.Instance.Initialize(PathMotionProfileDefault, usePreDockPose, useManualSpeed);
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetVisionMode(VisionMode mode, bool enable) {

    RobotEngineManager.Instance.Message.EnableVisionMode = Singleton<EnableVisionMode>.Instance.Initialize(mode, enable);
    RobotEngineManager.Instance.SendMessage();
  }

  public void RequestSetUnlock(Anki.Cozmo.UnlockId unlockID, bool unlocked) {
    RobotEngineManager.Instance.Message.RequestSetUnlock = Singleton<RequestSetUnlock>.Instance.Initialize(unlockID, unlocked);
    RobotEngineManager.Instance.SendMessage();
  }

  public void ExecuteReactionTrigger(Anki.Cozmo.ReactionTriggerToBehavior reactionTriggerToBehaviorEntry) {
    DAS.Debug(this, "ActivateReactionChooser: " + reactionTriggerToBehaviorEntry.ToString());

    RobotEngineManager.Instance.Message.ExecuteReactionTrigger = Singleton<ExecuteReactionTrigger>.Instance.Initialize(reactionTriggerToBehaviorEntry);
    RobotEngineManager.Instance.SendMessage();
  }

  public void ExecuteBehaviorByExecutableType(ExecutableBehaviorType type, int numTimesToExecute = -1) {
    DAS.Debug(this, "Execute Behavior " + type);

    if (type == ExecutableBehaviorType.LiftLoadTest) {
      RobotEngineManager.Instance.Message.SetLiftLoadTestAsActivatable = Singleton<SetLiftLoadTestAsActivatable>.Instance;
      RobotEngineManager.Instance.SendMessage();
    }

    RobotEngineManager.Instance.Message.ExecuteBehaviorByExecutableType =
             Singleton<ExecuteBehaviorByExecutableType>.Instance.Initialize(type, numTimesToExecute);
    RobotEngineManager.Instance.SendMessage();
  }

  public void ExecuteBehaviorByID(BehaviorID behaviorID, int numTimesToExecute = -1) {
    DAS.Debug(this, "Execute Behavior By Name" + behaviorID.ToString());

    if (behaviorID == BehaviorID.LiftLoadTest) {
      RobotEngineManager.Instance.Message.SetLiftLoadTestAsActivatable = Singleton<SetLiftLoadTestAsActivatable>.Instance;
      RobotEngineManager.Instance.SendMessage();
    }

    RobotEngineManager.Instance.Message.ExecuteBehaviorByID = Singleton<ExecuteBehaviorByID>.Instance.Initialize(behaviorID, numTimesToExecute);
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetEnableFreeplayActivity(bool enable) {
    if (enable) {
      ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Freeplay);
    }
    else {
      ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Selection);
      ExecuteBehaviorByExecutableType(Anki.Cozmo.ExecutableBehaviorType.Wait);
    }
  }

  public void ActivateHighLevelActivity(HighLevelActivity activityType) {
    DAS.Debug(this, "ActivateBehaviorChooser: " + activityType);

    RobotEngineManager.Instance.Message.ActivateHighLevelActivity =
      Singleton<ActivateHighLevelActivity>.Instance.Initialize(activityType);

    RobotEngineManager.Instance.SendMessage();
  }
  public void RequestReactionTriggerMap() {
    DAS.Debug(this, "Requested Reaction Trigger Map");

    RobotEngineManager.Instance.Message.RequestReactionTriggerMap = Singleton<RequestReactionTriggerMap>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public void RequestAllBehaviorsList() {
    DAS.Debug(this, "Requested Chooser Behavior List");

    RobotEngineManager.Instance.Message.RequestAllBehaviorsList =
                                     Singleton<RequestAllBehaviorsList>.Instance;
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

  // should only be called from update loop
  private void SetAllBackpackLEDs() {
    for (int i = 0; i < BackpackLights.Length; i++) {
      Singleton<SetBackpackLEDs>.Instance.onColor[i] = BackpackLights[i].OnColor;
      Singleton<SetBackpackLEDs>.Instance.offColor[i] = BackpackLights[i].OffColor;
      Singleton<SetBackpackLEDs>.Instance.onPeriod_ms[i] = BackpackLights[i].OnPeriodMs;
      Singleton<SetBackpackLEDs>.Instance.offPeriod_ms[i] = BackpackLights[i].OffPeriodMs;
      Singleton<SetBackpackLEDs>.Instance.transitionOnPeriod_ms[i] = BackpackLights[i].TransitionOnPeriodMs;
      Singleton<SetBackpackLEDs>.Instance.transitionOffPeriod_ms[i] = BackpackLights[i].TransitionOffPeriodMs;
    }

    RobotEngineManager.Instance.Message.SetBackpackLEDs = Singleton<SetBackpackLEDs>.Instance;
    RobotEngineManager.Instance.SendMessage();

    SetLastLEDs();
  }

  private void SetLastLEDs() {
    for (int i = 0; i < BackpackLights.Length; ++i) {
      BackpackLights[i].SetLastInfo();
    }
  }

  #endregion

  public void SetEnableFreeplayLightStates(bool enable, int objectID = -1) {
    DAS.Debug(this, "SetEnableFreeplayLightStates: " + enable);

    RobotEngineManager.Instance.Message.EnableLightStates = Singleton<EnableLightStates>.Instance.Initialize(enable, objectID);
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

  public void UpdateLightMessages(bool now = false) {
    if (RobotEngineManager.Instance == null || !RobotEngineManager.Instance.IsConnectedToEngine)
      return;

    if (Time.time > ActiveObject.Light.MessageDelay || now) {
      var enumerator = LightCubes.GetEnumerator();

      while (enumerator.MoveNext()) {
        LightCube lightCube = enumerator.Current.Value;

        if (lightCube.LightsChanged)
          lightCube.SetAllLEDs();
      }

      if ((Charger != null) && Charger.LightsChanged) {
        Charger.SetAllLEDs();
      }
    }

    if (Time.time > Light.MessageDelay || now) {
      if (_LightsChanged)
        SetAllBackpackLEDs();
    }
  }

  #endregion

  public uint SayTextWithEvent(string text, AnimationTrigger playEvent, SayTextIntent intent = SayTextIntent.Text, bool fitToDuration = false, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    DAS.Debug(this, "Saying text: " + PrivacyGuard.HidePersonallyIdentifiableInfo(text));
    return SendQueueSingleAction(Singleton<SayTextWithIntent>.Instance.Initialize(text, playEvent, intent, fitToDuration), callback, queueActionPosition);
  }

  public void EraseAllEnrolledFaces() {
    RobotEngineManager.Instance.Message.EraseAllEnrolledFaces = Singleton<EraseAllEnrolledFaces>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public void EraseEnrolledFaceByID(int faceID) {
    RobotEngineManager.Instance.Message.EraseEnrolledFaceByID = Singleton<EraseEnrolledFaceByID>.Instance.Initialize(faceID);
    RobotEngineManager.Instance.SendMessage();
  }

  public void UpdateEnrolledFaceByID(int faceID, string oldFaceName, string newFaceName) {
    RobotEngineManager.Instance.Message.UpdateEnrolledFaceByID = Singleton<UpdateEnrolledFaceByID>.Instance.Initialize(faceID, oldFaceName, newFaceName);
    RobotEngineManager.Instance.SendMessage();
  }

  public void LoadFaceAlbumFromFile(string path, bool isPathRelative = true) {
    RobotEngineManager.Instance.Message.LoadFaceAlbumFromFile = Singleton<LoadFaceAlbumFromFile>.Instance.Initialize(path, isPathRelative);
    RobotEngineManager.Instance.SendMessage();
  }

  public void DisableAllReactionsWithLock(string id) {
    DisableReactionsWithLock(id, _AllTriggers);
  }

  public void DisableReactionsWithLock(string id, AllTriggersConsidered triggerFlags) {
    RobotEngineManager.Instance.Message.DisableReactionsWithLock = _RequestDisableReactions.Initialize(id, triggerFlags);
    RobotEngineManager.Instance.SendMessage();
  }

  public void RemoveDisableReactionsLock(string id) {
    RobotEngineManager.Instance.Message.RemoveDisableReactionsLock = _RequestReEnableReactions.Initialize(id);
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandleInFieldOfViewStateChanged(ObservableObject objectChanged,
                                               ActiveObject.InFieldOfViewState oldState,
                                               ActiveObject.InFieldOfViewState newState) {
    VisibleLightCubes.Clear();
    foreach (KeyValuePair<int, LightCube> kvp in LightCubes) {
      if (kvp.Value.IsInFieldOfView) {
        VisibleLightCubes.Add(kvp.Key, kvp.Value);
      }
    }

    VisibleObjects.Clear();
    foreach (KeyValuePair<int, ObservableObject> kvp in KnownObjects) {
      if (kvp.Value.IsInFieldOfView) {
        VisibleObjects.Add(kvp.Key, kvp.Value);
      }
    }
  }

  public void EnableDroneMode(bool enable) {
    RobotEngineManager.Instance.Message.EnableDroneMode = Singleton<EnableDroneMode>.Instance.Initialize(enable);
    RobotEngineManager.Instance.SendMessage();
  }

  public void RestoreRobotFromBackup(uint backupToRestoreFrom) {
    RobotEngineManager.Instance.Message.RestoreRobotFromBackup = Singleton<RestoreRobotFromBackup>.Instance.Initialize(backupToRestoreFrom);
    RobotEngineManager.Instance.SendMessage();
  }

  public void WipeRobotGameData() {
    RobotEngineManager.Instance.Message.WipeRobotGameData = Singleton<WipeRobotGameData>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public void WipeRobotNeedsData() {
    RobotEngineManager.Instance.Message.WipeRobotNeedsData = Singleton<WipeRobotNeedsData>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public void WipeDeviceNeedsData(bool reinitializeNeeds) {
    RobotEngineManager.Instance.Message.WipeDeviceNeedsData = Singleton<WipeDeviceNeedsData>.Instance;
    RobotEngineManager.Instance.Message.WipeDeviceNeedsData.reinitializeNeeds = reinitializeNeeds;
    RobotEngineManager.Instance.SendMessage();
  }

  public void RequestRobotRestoreData() {
    RobotEngineManager.Instance.Message.RequestRobotRestoreData = Singleton<RequestRobotRestoreData>.Instance;
    RobotEngineManager.Instance.SendMessage();
  }

  public void NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag tag, byte[] data, byte index, byte numTotalBlobs) {
    RobotEngineManager.Instance.Message.NVStorageWriteEntry = Singleton<NVStorageWriteEntry>.Instance.Initialize(tag, data, index, numTotalBlobs);
    RobotEngineManager.Instance.SendMessage();
  }

  public void EnableCubeSleep(bool enable, bool skipAnimation = false) {
    RobotEngineManager.Instance.Message.EnableCubeSleep = Singleton<EnableCubeSleep>.Instance.Initialize(enable, skipAnimation);
    RobotEngineManager.Instance.SendMessage();
  }

  public void EnableCubeLightsStateTransitionMessages(bool enable) {
    RobotEngineManager.Instance.Message.EnableCubeLightsStateTransitionMessages = Singleton<EnableCubeLightsStateTransitionMessages>.Instance.Initialize(enable);
    RobotEngineManager.Instance.SendMessage();
  }

  public void FlashCurrentLightsState(uint objectId) {
    RobotEngineManager.Instance.Message.FlashCurrentLightsState = Singleton<FlashCurrentLightsState>.Instance.Initialize(objectId);
    RobotEngineManager.Instance.SendMessage();
  }

  public void EnableLift(bool enable) {
    RobotEngineManager.Instance.Message.EnableLiftPower = Singleton<EnableLiftPower>.Instance.Initialize(enable);
    RobotEngineManager.Instance.SendMessage();
  }

  public void EnterSDKMode(bool isExternalSdkMode) {
    RobotEngineManager.Instance.Message.EnterSdkMode = Singleton<EnterSdkMode>.Instance.Initialize(isExternalSdkMode);
    RobotEngineManager.Instance.SendMessage();
  }

  public void ExitSDKMode(bool isExternalSdkMode) {
    RobotEngineManager.Instance.Message.ExitSdkMode = Singleton<ExitSdkMode>.Instance.Initialize(isExternalSdkMode);
    RobotEngineManager.Instance.SendMessage();
  }

  public void SetNightVision(bool enable) {
    RobotEngineManager.Instance.Message.SetHeadlight = Singleton<SetHeadlight>.Instance.Initialize(enable);
    RobotEngineManager.Instance.SendMessage();
  }

  public uint PlayCubeAnimationTrigger(ObservableObject obj, CubeAnimationTrigger trigger, RobotCallback callback = null) {
    return SendQueueSingleAction(Singleton<PlayCubeAnimationTrigger>.Instance.Initialize(obj, trigger), callback, QueueActionPosition.IN_PARALLEL);
  }

  public uint WaitAction(float waitTime_s, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SendQueueSingleAction(Singleton<Wait>.Instance.Initialize(waitTime_s), callback, queueActionPosition);
  }

  public uint DisplayFaceImage(uint duration_ms, byte[] faceData, RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SendQueueSingleAction(Singleton<DisplayFaceImage>.Instance.Initialize(duration_ms, faceData), callback, queueActionPosition);
  }

  public uint DriveOffChargerContacts(RobotCallback callback = null, QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
    return SendQueueSingleAction(Singleton<DriveOffChargerContacts>.Instance, callback, queueActionPosition);
  }

}
