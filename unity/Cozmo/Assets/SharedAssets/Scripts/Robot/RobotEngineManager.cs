using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Anki.Cozmo;
using Anki.Cozmo.Audio;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

/// <summary>
/// Robot engine manager lives on a GameObject(named MasterObject) in our Intro scene,
/// and handles launching, ticking, and messaging with the Cozmo Engine
/// </summary>
public class RobotEngineManager : MonoBehaviour {

  public const string kRobotIP = "172.31.1.1";

  public static RobotEngineManager Instance = null;

  public Dictionary<int, IRobot> Robots { get; private set; }

  // Cache the last current robot
  private int _LastCurrentRobotID;
  private IRobot _LastCurrentRobot;

  public int CurrentRobotID { get; private set; }

  public IRobot CurrentRobot {
    get {
      if (_LastCurrentRobot != null && _LastCurrentRobotID == CurrentRobotID) {
        return _LastCurrentRobot;
      }
      IRobot current;
      if (Robots.TryGetValue(CurrentRobotID, out current)) {
        _LastCurrentRobot = current;
        _LastCurrentRobotID = CurrentRobotID;
        return current;
      }
      return null;
    }
  }

  public bool IsConnected { get { return (_Channel != null && _Channel.IsConnected); } }

  private List<string> _RobotAnimationNames = new List<string>();

  [SerializeField]
  private TextAsset _Configuration;

  [SerializeField]
  private TextAsset _AlternateConfiguration;

  private DisconnectionReason _LastDisconnectionReason = DisconnectionReason.None;

  public event Action<string> ConnectedToClient;
  public event Action<DisconnectionReason> DisconnectedFromClient;
  public event Action<int> RobotConnected;
  public event Action<uint, bool, RobotActionType> SuccessOrFailure;
  public event Action<bool, string> RobotCompletedAnimation;
  public event Action<bool, uint> RobotCompletedCompoundAction;
  public event Action<bool, uint> RobotCompletedTaggedAction;
  public event Action<int, string, Vector3, Quaternion> RobotObservedFace;
  public event Action<int, Vector3, Quaternion> RobotObservedNewFace;
  public event Action<Anki.Cozmo.ExternalInterface.RobotObservedPossibleObject> OnObservedPossibleObject;
  public event Action<Anki.Cozmo.EmotionType, float> OnEmotionRecieved;
  public event Action<Anki.Cozmo.ProgressionStatType, int> OnProgressionStatRecieved;
  public event Action<Vector2> OnObservedMotion;
  public event Action OnRobotPickedUp;
  public event Action OnRobotPutDown;
  public event Action<Anki.Cozmo.CliffEvent> OnCliffEvent;
  public event Action OnCliffEventFinished;
  public event Action<G2U.RobotOnBack> OnRobotOnBack;
  public event Action OnRobotOnBackFinished;
  public event Action<Anki.Cozmo.ExternalInterface.RequestGameStart> OnRequestGameStart;
  public event Action<Anki.Cozmo.ExternalInterface.DenyGameStart> OnDenyGameStart;
  public event Action<Anki.Cozmo.ExternalInterface.InitBlockPoolMessage> OnInitBlockPoolMsg;
  public event Action<Anki.Cozmo.ExternalInterface.ObjectAvailable> OnObjectAvailableMsg;
  public event Action<Anki.Cozmo.ExternalInterface.ObjectUnavailable> OnObjectUnavailableMsg;
  public event Action<ImageChunk> OnImageChunkReceived;
  public event Action<Anki.Cozmo.ExternalInterface.FactoryTestResult> OnFactoryResult;
  public event Action<Anki.Cozmo.UnlockId, bool> OnRequestSetUnlockResult;
  public event Action<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress> OnFirmwareUpdateProgress;
  public event Action<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete> OnFirmwareUpdateComplete;
  public event Action OnSparkUnlockEnded;
  public event Action<Anki.Cozmo.ExternalInterface.NVStorageData> OnGotNVStorageData;
  public event Action<Anki.Cozmo.ExternalInterface.NVStorageOpResult> OnGotNVStorageOpResult;
  public event Action<Anki.Cozmo.ExternalInterface.DebugLatencyMessage> OnDebugLatencyMsg;
  public event Action<Anki.Cozmo.ExternalInterface.RequestEnrollFace> OnRequestEnrollFace;
  public event Action<int> OnDemoState;
  public event Action<Anki.Cozmo.ExternalInterface.AnimationEvent> OnRobotAnimationEvent;
  public event Action<Anki.Cozmo.ObjectConnectionState> OnObjectConnectionState;
  public event Action<int, string> OnRobotLoadedKnownFace;

  #region Audio Callback events

  public event Action<AudioCallback> ReceivedAudioCallback;

  #endregion

  private bool _CozmoBindingStarted = false;
  private RobotUdpChannel _Channel = null;
  private bool _IsRobotConnected = false;

  private const int _UIDeviceID = 1;
  private const int _UIAdvertisingRegistrationPort = 5103;
  // 0 means random unused port
  // Used to be 5106
  private const int _UILocalPort = 0;

  public bool AllowImageSaving { get; private set; }

  private RobotMessageOut _MessageOut = new RobotMessageOut();

  public U2G.MessageGameToEngine Message { get { return _MessageOut.Message; } }

  private U2G.StartEngine StartEngineMessage = new U2G.StartEngine();
  private U2G.ConnectToRobot ConnectToRobotMessage = new U2G.ConnectToRobot();
  private U2G.ConnectToUiDevice ConnectToUiDeviceMessage = new U2G.ConnectToUiDevice(UiConnectionType.UI, 0);

  private U2G.GetAllDebugConsoleVarMessage _GetAllDebugConsoleVarMessage = new U2G.GetAllDebugConsoleVarMessage();
  private U2G.SetDebugConsoleVarMessage _SetDebugConsoleVarMessage = new U2G.SetDebugConsoleVarMessage();
  private U2G.RunDebugConsoleFuncMessage _RunDebugConsoleFuncMessage = new U2G.RunDebugConsoleFuncMessage();
  private U2G.DenyGameStart _DenyGameStartMessage = new U2G.DenyGameStart();

  public bool InitSkillSystem;

  private void OnEnable() {
    DAS.Event("RobotEngineManager.OnEnable", string.Empty);
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
    }

    TextAsset config = _Configuration;

    if (PlayerPrefs.GetInt("DebugUseAltConfig", 0) == 1) {
      config = _AlternateConfiguration;
    }

    if (config == null) {
      DAS.Error("RobotEngineManager.ErrorInitializingCozmoBinding.NoConfig", string.Empty);
    }
    else {
      string configuration = AddDataPlatformPathsToConfiguration(config.text);

      CozmoBinding.Startup(configuration);
      _CozmoBindingStarted = true;
    }

    Application.runInBackground = true;

    _Channel = new RobotUdpChannel();
    _Channel.ConnectedToClient += Connected;
    _Channel.DisconnectedFromClient += Disconnected;
    _Channel.MessageReceived += ReceivedMessage;

    Robots = new Dictionary<int, IRobot>();

    // Startup Singletons depending on RobotEvents
    if (InitSkillSystem) {
      SkillSystem.Instance.InitInstance();
    }

  }

  private void OnDisable() {

    if (_Channel != null) {
      if (_Channel.IsActive) {
        Disconnect();
        Disconnected(DisconnectionReason.UnityReloaded);
      }

      _Channel = null;
    }

    if (_CozmoBindingStarted) {
      CozmoBinding.Shutdown();
    }

    // Destroy Singletons depending on RobotEvents
    SkillSystem.Instance.DestroyInstance();
  }

  void Update() {
    if (_Channel != null) {
      _Channel.Update();
    }

    if (CurrentRobot is MockRobot) {
      (CurrentRobot as MockRobot).UpdateCallbackTicks();
    }
  }

  public void LateUpdate() {
    if (CurrentRobot == null)
      return;

    CurrentRobot.UpdateLightMessages();
  }

  public List<string> GetRobotAnimationNames() {
    return _RobotAnimationNames;
  }

  public void AddRobot(byte robotID) {
    IRobot oldRobot;
    if (Robots.TryGetValue(robotID, out oldRobot)) {
      if (oldRobot != null) {
        oldRobot.Dispose();
      }
      _LastCurrentRobot = null;
      Robots.Remove(robotID);
    }

    IRobot robot = new Robot(robotID);
    Robots.Add(robotID, robot);
    CurrentRobotID = robotID;
  }

  public void Connect(string engineIP) {
    _Channel.Connect(_UIDeviceID, _UILocalPort, engineIP, _UIAdvertisingRegistrationPort);
  }

  public void Disconnect() {
    _IsRobotConnected = false;
    if (_Channel != null) {
      _Channel.Disconnect();

      // only really needed in editor in case unhitting play
#if UNITY_EDITOR
      float limit = Time.realtimeSinceStartup + 2.0f;
      while (_Channel.HasPendingOperations) {
        if (limit < Time.realtimeSinceStartup) {
          DAS.Warn("RobotEngineManager.NotWaitingForDisconnectToFinishSending", string.Empty);
          break;
        }
        System.Threading.Thread.Sleep(500);
      }
#endif
    }
  }

  public DisconnectionReason GetLastDisconnectionReason() {
    DisconnectionReason reason = _LastDisconnectionReason;
    _LastDisconnectionReason = DisconnectionReason.None;
    return reason;
  }

  private void Connected(string connectionIdentifier) {
    if (ConnectedToClient != null) {
      ConnectedToClient(connectionIdentifier);
    }
  }

  private void Disconnected(DisconnectionReason reason) {
    DAS.Debug("RobotEngineManager.Disconnected", reason.ToString());
    _IsRobotConnected = false;

    _LastDisconnectionReason = reason;
    if (DisconnectedFromClient != null) {
      DisconnectedFromClient(reason);
    }
  }

  public void SendMessage() {
    if (!IsConnected) {
      DAS.Warn("RobotEngineManager.MessageNotSent", "Not Connected");
      return;
    }

    _Channel.Send(_MessageOut);
  }

  private void ReceivedMessage(RobotMessageIn messageIn) {
    var message = messageIn.Message;
    switch (message.GetTag()) {
    case G2U.MessageEngineToGame.Tag.Ping:
      ReceivedSpecificMessage(message.Ping);
      break;
    case G2U.MessageEngineToGame.Tag.UiDeviceAvailable:
      ReceivedSpecificMessage(message.UiDeviceAvailable);
      break;
    case G2U.MessageEngineToGame.Tag.RobotConnected:
      ReceivedSpecificMessage(message.RobotConnected);
      break;
    case G2U.MessageEngineToGame.Tag.UiDeviceConnected:
      ReceivedSpecificMessage(message.UiDeviceConnected);
      break;
    case G2U.MessageEngineToGame.Tag.RobotDisconnected:
      ReceivedSpecificMessage(message.RobotDisconnected);
      break;
    case G2U.MessageEngineToGame.Tag.EngineRobotCLADVersionMismatch:
      ReceivedSpecificMessage(message.EngineRobotCLADVersionMismatch);
      break;

    // Block pool and connection to objects
    case G2U.MessageEngineToGame.Tag.ObjectAvailable:
      ReceivedSpecificMessage(message.ObjectAvailable);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectUnavailable:
      ReceivedSpecificMessage(message.ObjectUnavailable);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectConnectionState:
      ReceivedSpecificMessage(message.ObjectConnectionState);
      break;
    // end Block pool and connection to objects

    // Vision messages 
    case G2U.MessageEngineToGame.Tag.RobotObservedNothing:
      // TODO remove this message from engine
      break;
    case G2U.MessageEngineToGame.Tag.RobotProcessedImage:
      ReceiveSpecificMessage(message.RobotProcessedImage);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedObject:
      ReceivedSpecificMessage(message.RobotObservedObject);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedPossibleObject:
      ReceivedSpecificMessage(message.RobotObservedPossibleObject);
      break;
    case G2U.MessageEngineToGame.Tag.RobotDeletedObject:
      ReceivedSpecificMessage(message.RobotDeletedObject);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectMoved:
      ReceivedSpecificMessage(message.ObjectMoved);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectStoppedMoving:
      ReceivedSpecificMessage(message.ObjectStoppedMoving);
      break;
    case G2U.MessageEngineToGame.Tag.RobotMarkedObjectPoseUnknown:
      ReceivedSpecificMessage(message.RobotMarkedObjectPoseUnknown);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedFace:
      ReceivedSpecificMessage(message.RobotObservedFace);
      break;
    case G2U.MessageEngineToGame.Tag.RobotDeletedFace:
      ReceivedSpecificMessage(message.RobotDeletedFace);
      break;
    case G2U.MessageEngineToGame.Tag.RobotObservedMotion:
      ReceivedSpecificMessage(message.RobotObservedMotion);
      break;
    // End vision messages 

    case G2U.MessageEngineToGame.Tag.PlaySound:
      ReceivedSpecificMessage(message.PlaySound);
      break;
    case G2U.MessageEngineToGame.Tag.StopSound:
      ReceivedSpecificMessage(message.StopSound);
      break;
    case G2U.MessageEngineToGame.Tag.RobotState:
      ReceivedSpecificMessage(message.RobotState);
      break;
    case G2U.MessageEngineToGame.Tag.RobotCompletedAction:
      ReceivedSpecificMessage(message.RobotCompletedAction);
      break;
    case G2U.MessageEngineToGame.Tag.ObjectTapped:
      ReceivedSpecificMessage(message.ObjectTapped);
      break;
    case G2U.MessageEngineToGame.Tag.AnimationAvailable:
      ReceivedSpecificMessage(message.AnimationAvailable);
      break;
    case G2U.MessageEngineToGame.Tag.RobotPickedUp:
      ReceivedSpecificMessage(message.RobotPickedUp);
      break;
    case G2U.MessageEngineToGame.Tag.RobotPutDown:
      ReceivedSpecificMessage(message.RobotPutDown);
      break;
    case G2U.MessageEngineToGame.Tag.MoodState:
      ReceivedSpecificMessage(message.MoodState);
      break;
    // Audio Callbacks
    case G2U.MessageEngineToGame.Tag.AudioCallback:
      ReceivedSpecificMessage(message.AudioCallback);
      break;
    case G2U.MessageEngineToGame.Tag.InitDebugConsoleVarMessage:
      ReceivedSpecificMessage(message.InitDebugConsoleVarMessage);
      break;
    case G2U.MessageEngineToGame.Tag.VerifyDebugConsoleVarMessage:
      ReceivedSpecificMessage(message.VerifyDebugConsoleVarMessage);
      break;
    case G2U.MessageEngineToGame.Tag.CliffEvent:
      ReceivedSpecificMessage(message.CliffEvent);
      break;
    case G2U.MessageEngineToGame.Tag.RobotCliffEventFinished:
      ReceivedSpecificMessage(message.RobotCliffEventFinished);
      break;
    case G2U.MessageEngineToGame.Tag.DebugString:
      ReceivedSpecificMessage(message.DebugString);
      break;
    case G2U.MessageEngineToGame.Tag.DebugAnimationString:
      ReceivedSpecificMessage(message.DebugAnimationString);
      break;
    case G2U.MessageEngineToGame.Tag.DebugLatencyMessage:
      ReceivedSpecificMessage(message.DebugLatencyMessage);
      break;
    case G2U.MessageEngineToGame.Tag.RequestGameStart:
      ReceivedSpecificMessage(message.RequestGameStart);
      break;
    case G2U.MessageEngineToGame.Tag.DenyGameStart:
      ReceivedSpecificMessage(message.DenyGameStart);
      break;
    case G2U.MessageEngineToGame.Tag.InitBlockPoolMessage:
      ReceivedSpecificMessage(message.InitBlockPoolMessage);
      break;
    case G2U.MessageEngineToGame.Tag.ImageChunk:
      ReceivedSpecificMessage(message.ImageChunk);
      break;
    case G2U.MessageEngineToGame.Tag.UnlockStatus:
      ReceivedSpecificMessage(message.UnlockStatus);
      break;
    case G2U.MessageEngineToGame.Tag.RequestSetUnlockResult:
      ReceivedSpecificMessage(message.RequestSetUnlockResult);
      break;
    case G2U.MessageEngineToGame.Tag.FactoryTestResult:
      ReceivedSpecificMessage(message.FactoryTestResult);
      break;
    case G2U.MessageEngineToGame.Tag.FirmwareUpdateProgress:
      ReceivedSpecificMessage(message.FirmwareUpdateProgress);
      break;
    case G2U.MessageEngineToGame.Tag.FirmwareUpdateComplete:
      ReceivedSpecificMessage(message.FirmwareUpdateComplete);
      break;
    case G2U.MessageEngineToGame.Tag.SparkUnlockEnded:
      ReceivedSpecificMessage(message.SparkUnlockEnded);
      break;
    case G2U.MessageEngineToGame.Tag.NVStorageData:
      ReceivedSpecificMessage(message.NVStorageData);
      break;
    case G2U.MessageEngineToGame.Tag.NVStorageOpResult:
      ReceivedSpecificMessage(message.NVStorageOpResult);
      break;
    case G2U.MessageEngineToGame.Tag.RequestEnrollFace:
      ReceivedSpecificMessage(message.RequestEnrollFace);
      break;
    case G2U.MessageEngineToGame.Tag.DemoState:
      ReceivedSpecificMessage(message.DemoState);
      break;
    case G2U.MessageEngineToGame.Tag.AnimationEvent:
      ReceiveSpecificMessage(message.AnimationEvent);
      break;
    case G2U.MessageEngineToGame.Tag.RobotOnBack:
      ReceivedSpecificMessage(message.RobotOnBack);
      break;
    case G2U.MessageEngineToGame.Tag.RobotOnBackFinished:
      ReceivedSpecificMessage(message.RobotOnBackFinished);
      break;
    case G2U.MessageEngineToGame.Tag.RobotLoadedKnownFace:
      ReceiveSpecificMessage(message.RobotLoadedKnownFace);
      break;
    default:
      DAS.Warn("RobotEngineManager.ReceiveUnsupportedMessage", message.GetTag() + " is not supported");
      break;
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.Ping message) {
    if (message.isResponse) {
      DAS.Warn("Unity.ReceivedResponsePing", "Unity receiving response pings is unsupported");
    }
    else {
      Message.Ping = Singleton<U2G.Ping>.Instance.Initialize(counter: message.counter, timeSent_ms: message.timeSent_ms, isResponse: true);
      SendMessage();
    }
  }

  private void ReceivedSpecificMessage(G2U.DemoState message) {
    if (OnDemoState != null) {
      OnDemoState(message.stateNum);
    }
  }

  private void ReceivedSpecificMessage(G2U.RequestEnrollFace message) {
    if (OnRequestEnrollFace != null) {
      OnRequestEnrollFace(message);
    }
  }

  private void ReceiveSpecificMessage(Anki.Cozmo.ExternalInterface.AnimationEvent message) {
    if (OnRobotAnimationEvent != null) {
      OnRobotAnimationEvent(message);
    }
  }

  private void ReceivedSpecificMessage(G2U.UiDeviceAvailable message) {
    ConnectToUiDeviceMessage.deviceID = (byte)message.deviceID;

    Message.ConnectToUiDevice = ConnectToUiDeviceMessage;
    SendMessage();
  }

  private void ReceivedSpecificMessage(G2U.RobotConnected message) {
    if (!_IsRobotConnected) {
      _IsRobotConnected = true;

      AddRobot((byte)message.robotID);

      if (RobotConnected != null) {
        RobotConnected((byte)message.robotID);
      }
    }
  }

  private void ReceivedSpecificMessage(G2U.UiDeviceConnected message) {
    DAS.Debug("RobotEngineManager.DeviceConnected", "Device connected: " + message.connectionType.ToString() + "," + message.deviceID.ToString());
  }

  private void ReceivedSpecificMessage(G2U.RobotDisconnected message) {
    DAS.Error("RobotEngineManager.RobotDisconnected", "Robot " + message.robotID + " disconnected after " + message.timeSinceLastMsg_sec.ToString("0.00") + " seconds.");
    Disconnect();
    Disconnected(DisconnectionReason.RobotDisconnected);
  }

  private void ReceivedSpecificMessage(G2U.EngineRobotCLADVersionMismatch message) {
    if (message.engineToRobotMismatch) {
      string str = "Engine to Robot CLAD version mismatch. Engine's EngineToRobot hash = " + message.engineEnginetoRobotHash + ". Robot's EngineToRobot hash = " + message.robotEnginetoRobotHash;
      DAS.Error("clad_version_mismatch_engine_to_robot", str);
    }

    if (message.robotToEngineMistmatch) {
      string str = "Robot to Engine CLAD version mismatch. Engine's EngineToRobot hash = " + message.engineRobotToEngineHash + ". Robot's RobotToEngine hash = " + message.robotRobotToEngineHash;
      DAS.Error("clad_version_mismatch_robot_to_engine", str);
    }

    //DebugMenuManager.Instance.OnDebugMenuButtonTap();
  }

  private void ReceivedSpecificMessage(G2U.InitDebugConsoleVarMessage message) {
    DAS.Info("RobotEngineManager.ReceivedDebugConsoleInit", " Recieved Debug Console Init");
    for (int i = 0; i < message.varData.Length; ++i) {
      Anki.Debug.DebugConsoleData.Instance.AddConsoleVar(message.varData[i]);
    }
  }

  private void ReceivedSpecificMessage(G2U.VerifyDebugConsoleVarMessage message) {
    Anki.Debug.DebugConsoleData.Instance.SetStatusText(message.statusMessage);
  }

  private void ReceivedSpecificMessage(ObjectTapped message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      CurrentRobot.HandleObservedObjectTapped(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ObjectConnectionState message) {
    if (OnObjectConnectionState != null) {
      OnObjectConnectionState(message);
    }
  }

  /// <summary>
  /// Message sent when an ObservedObject is deleted.
  /// </summary>
  private void ReceivedSpecificMessage(G2U.RobotDeletedObject message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      CurrentRobot.DeleteObservedObject((int)message.objectID);
    }
  }

  /// <summary>
  /// Message sent when vision system finishes processing a full frame. Other messages
  /// may be called depending on what is found. This is sent _after_ those messages.
  /// </summary>
  private void ReceiveSpecificMessage(G2U.RobotProcessedImage message) {
    // TODO: Check if robot id matches current robot (message doesn't have id)
    if (CurrentRobot != null) {
      CurrentRobot.FinishedProcessingImage(message.timestamp);
    }
  }

  /// <summary>
  /// Message sent for each ObservedObject identified in a vision frame.
  /// </summary>
  private void ReceivedSpecificMessage(G2U.RobotObservedObject message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      CurrentRobot.HandleSeeObservedObject(message);
    }
  }

  /// <summary>
  /// Message sent when a vision frame finds a marker that is too far away to 
  /// be identified as a particular ObservedObject.
  /// </summary>
  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.RobotObservedPossibleObject message) {
    if (OnObservedPossibleObject != null) {
      OnObservedPossibleObject(message);
    }
  }

  /// <summary>
  /// Message sent when an ObservedObject is moved.
  /// </summary>
  private void ReceivedSpecificMessage(ObjectMoved message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      CurrentRobot.HandleObservedObjectMoved(message);
    }
  }

  /// <summary>
  /// Message sent when an ObservedObject stops moving.
  /// </summary>
  private void ReceivedSpecificMessage(ObjectStoppedMoving message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      CurrentRobot.HandleObservedObjectStoppedMoving(message);
    }
  }

  /// <summary>
  /// Message sent when an ObservedObject's pose is unknown, which may happen when
  /// the robot does not see a known/dirty ObservedObject in the place it expected it to be.
  /// </summary>
  private void ReceivedSpecificMessage(G2U.RobotMarkedObjectPoseUnknown message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      CurrentRobot.HandleObservedObjectPoseUnknown((int)message.objectID);
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotObservedFace message) {
    if (CurrentRobot == null)
      return;
    CurrentRobot.UpdateObservedFaceInfo(message);

    if (message.faceID > 0 && message.name == "" && RobotObservedNewFace != null) {
      RobotObservedNewFace(message.faceID,
        new Vector3(message.world_x, message.world_y, message.world_z),
        new Quaternion(message.quaternion_x, message.quaternion_y, message.quaternion_z, message.quaternion_w));
    }

    if (RobotObservedFace != null) {
      RobotObservedFace(message.faceID, message.name,
        new Vector3(message.world_x, message.world_y, message.world_z),
        new Quaternion(message.quaternion_x, message.quaternion_y, message.quaternion_z, message.quaternion_w));
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotDeletedFace message) {
    if (CurrentRobot != null && CurrentRobot.ID == message.robotID) {
      int index = -1;
      for (int i = 0; i < CurrentRobot.Faces.Count; i++) {
        if (CurrentRobot.Faces[i].ID == message.faceID) {
          index = i;
          break;
        }
      }

      if (index != -1) {
        CurrentRobot.Faces.RemoveAt(index);
      }
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.RobotObservedMotion message) {
    if (CurrentRobot == null)
      return;

    if (OnObservedMotion != null) {
      var resolution = ImageResolutionTable.GetDimensions(ImageResolution.CVGA);
      // Normalize our position to [-1,1], [-1,1]
      OnObservedMotion(new Vector2(message.img_x * 2.0f / (float)resolution.Width, message.img_y * 2.0f / (float)resolution.Height));
    }

  }

  private void ReceivedSpecificMessage(G2U.RobotCompletedAction message) {
    if (CurrentRobot == null)
      return;

    RobotActionType actionType = (RobotActionType)message.actionType;
    bool success = message.result == ActionResult.SUCCESS;
    CurrentRobot.TargetLockedObject = null;

    CurrentRobot.SetLocalBusyTimer(0f);

    if (SuccessOrFailure != null) {
      SuccessOrFailure(message.idTag, success, actionType);
    }

    if (actionType == RobotActionType.PLAY_ANIMATION) {
      if (message.completionInfo.GetTag() == ActionCompletedUnion.Tag.animationCompleted) {
        // Reset cozmo's face
        string animationCompleted = message.completionInfo.animationCompleted.animationName;

        if (RobotCompletedAnimation != null) {
          RobotCompletedAnimation(success, animationCompleted);
        }
      }
      else {
        DAS.Warn("RobotEngineManager.ReceivedSpecificMessage(G2U.RobotCompletedAction message)",
          string.Format("Message is of type RobotActionType.PLAY_ANIMATION but message.completionInfo has tag {0} instead of ActionCompletedUnion.Tag.animationCompleted! idTag={1} result={2}",
            message.completionInfo.GetTag(), message.idTag, message.result));
      }
    }

    if (actionType == RobotActionType.COMPOUND) {
      if (RobotCompletedCompoundAction != null) {
        RobotCompletedCompoundAction(success, message.idTag);
      }
    }
    else if (message.idTag > 0) {
      if (RobotCompletedTaggedAction != null) {
        RobotCompletedTaggedAction(success, message.idTag);
      }
    }
  }

  private void ReceivedSpecificMessage(G2U.AnimationAvailable message) {
    if (!_RobotAnimationNames.Contains(message.animName))
      _RobotAnimationNames.Add(message.animName);
  }

  private void ReceivedSpecificMessage(G2U.RobotPickedUp message) {
    if (CurrentRobot != null && CurrentRobotID == message.robotID && OnRobotPickedUp != null) {
      OnRobotPickedUp();
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotPutDown message) {
    if (CurrentRobot != null && CurrentRobotID == message.robotID && OnRobotPutDown != null) {
      OnRobotPutDown();
    }
  }

  private void ReceivedSpecificMessage(G2U.MoodState message) {
    if (CurrentRobot == null)
      return;

    if (message.emotionValues.Length != (int)EmotionType.Count) {
      DAS.Error("MoodStateMessage.emotionValues.BadLength", "Expected " + EmotionType.Count + " entries but got " + message.emotionValues.Length);
    }
    else {
      for (EmotionType i = 0; i < EmotionType.Count; ++i) {
        //DAS.Info("Mood", "Robot " + message.robotID.ToString() + ": Emotion '" + i + "' = " + message.emotionValues[(int)i]);
        if (OnEmotionRecieved != null) {
          OnEmotionRecieved(i, message.emotionValues[(int)i]);
        }
      }
    }
  }

  private void ReceivedSpecificMessage(G2U.PlaySound message) {

  }

  private void ReceivedSpecificMessage(G2U.StopSound message) {

  }

  private void ReceivedSpecificMessage(G2U.RobotState message) {
    Robots[message.robotID].UpdateInfo(message);
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.Audio.AudioCallback message) {
    if (ReceivedAudioCallback != null) {
      ReceivedAudioCallback(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.CliffEvent message) {
    if (OnCliffEvent != null) {
      OnCliffEvent(message);
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotCliffEventFinished message) {
    if (OnCliffEventFinished != null) {
      OnCliffEventFinished();
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotOnBack message) {
    if (OnRobotOnBack != null) {
      OnRobotOnBack(message);
    }
  }

  private void ReceivedSpecificMessage(G2U.RobotOnBackFinished message) {
    if (OnRobotOnBackFinished != null) {
      OnRobotOnBackFinished();
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.InitBlockPoolMessage message) {
    if (OnInitBlockPoolMsg != null) {
      OnInitBlockPoolMsg(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.ObjectAvailable message) {
    if (OnObjectAvailableMsg != null) {
      OnObjectAvailableMsg(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.ObjectUnavailable message) {
    if (OnObjectUnavailableMsg != null) {
      OnObjectUnavailableMsg(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    if (OnRequestGameStart != null) {
      OnRequestGameStart(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.DenyGameStart message) {
    if (OnDenyGameStart != null) {
      OnDenyGameStart(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.FactoryTestResult message) {
    if (OnFactoryResult != null) {
      OnFactoryResult(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.DebugString message) {
    if (CurrentRobot != null) {
      if (CurrentRobot.CurrentBehaviorString != message.text) {
        CurrentRobot.CurrentBehaviorString = message.text;
      }
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.DebugAnimationString message) {
    if (CurrentRobot != null) {
      if (CurrentRobot.CurrentDebugAnimationString != message.text) {
        CurrentRobot.CurrentDebugAnimationString = message.text;
      }
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.DebugLatencyMessage message) {
    if (OnDebugLatencyMsg != null) {
      OnDebugLatencyMsg(message);
    }
  }


  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.UnlockStatus message) {
    if (UnlockablesManager.Instance != null) {
      Dictionary<Anki.Cozmo.UnlockId, bool> loadedUnlockables = new Dictionary<UnlockId, bool>();
      for (int i = 0; i < message.unlocks.Length; ++i) {
        loadedUnlockables.Add(message.unlocks[i].unlockID, message.unlocks[i].unlocked);
      }
      UnlockablesManager.Instance.OnConnectLoad(loadedUnlockables);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.RequestSetUnlockResult message) {
    if (OnRequestSetUnlockResult != null) {
      OnRequestSetUnlockResult(message.unlockID, message.unlocked);
    }
  }

  private void ReceivedSpecificMessage(ImageChunk message) {
    if (OnImageChunkReceived != null) {
      OnImageChunkReceived(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    if (OnFirmwareUpdateProgress != null) {
      OnFirmwareUpdateProgress(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    if (OnFirmwareUpdateComplete != null) {
      OnFirmwareUpdateComplete(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.SparkUnlockEnded message) {
    if (OnSparkUnlockEnded != null) {
      OnSparkUnlockEnded();
    }
  }

  public void StartEngine() {
    Message.StartEngine = StartEngineMessage;
    SendMessage();
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.NVStorageData message) {
    if (OnGotNVStorageData != null) {
      OnGotNVStorageData(message);
    }
  }

  private void ReceivedSpecificMessage(Anki.Cozmo.ExternalInterface.NVStorageOpResult message) {
    if (OnGotNVStorageOpResult != null) {
      OnGotNVStorageOpResult(message);
    }
  }

  private void ReceiveSpecificMessage(Anki.Cozmo.ExternalInterface.RobotLoadedKnownFace message) {
    if (OnRobotLoadedKnownFace != null) {
      OnRobotLoadedKnownFace(message.faceID, message.name);
    }
  }

  public void UpdateFirmware(int firmwareVersion) {
    Message.UpdateFirmware = Singleton<U2G.UpdateFirmware>.Instance.Initialize(firmwareVersion);
    SendMessage();
  }

  public void InitDebugConsole() {
    Message.GetAllDebugConsoleVarMessage = _GetAllDebugConsoleVarMessage;
    // should get a G2U.InitDebugConsoleVarMessage back
    SendMessage();
  }

  public void SetDebugConsoleVar(string varName, string tryValue) {
    _SetDebugConsoleVarMessage.tryValue = tryValue;
    _SetDebugConsoleVarMessage.varName = varName;
    Message.SetDebugConsoleVarMessage = _SetDebugConsoleVarMessage;
    SendMessage();
  }

  public void RunDebugConsoleFuncMessage(string funcName, string funcArgs) {
    _RunDebugConsoleFuncMessage.funcName = funcName;
    _RunDebugConsoleFuncMessage.funcArgs = funcArgs;
    Message.RunDebugConsoleFuncMessage = _RunDebugConsoleFuncMessage;
    SendMessage();
  }

  public void SendDenyGameStart() {
    Message.DenyGameStart = _DenyGameStartMessage;
    SendMessage();
  }

  /// <summary>
  /// Forcibly adds a new robot.
  /// </summary>
  /// <param name="robotId">The robot identifier.</param>
  /// <param name="robotIP">The ip address the robot is connected to.</param>
  /// <param name="robotIsSimulated">Specify true for a simulated robot.</param>
  public void ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated) {
    if (robotID < 0 || robotID > 255) {
      throw new ArgumentException("ID must be between 0 and 255.", "robotID");
    }

    if (string.IsNullOrEmpty(robotIP)) {
      throw new ArgumentNullException("robotIP");
    }

    if (Encoding.UTF8.GetByteCount(robotIP) + 1 > ConnectToRobotMessage.ipAddress.Length) {
      throw new ArgumentException("IP address too long.", "robotIP");
    }
    int length = Encoding.UTF8.GetBytes(robotIP, 0, robotIP.Length, ConnectToRobotMessage.ipAddress, 0);
    ConnectToRobotMessage.ipAddress[length] = 0;

    ConnectToRobotMessage.robotID = (byte)robotID;
    ConnectToRobotMessage.isSimulated = robotIsSimulated ? (byte)1 : (byte)0;

    Message.ConnectToRobot = ConnectToRobotMessage;
    SendMessage();
  }

  private string AddDataPlatformPathsToConfiguration(string configuration) {
    StringBuilder sb = new StringBuilder(configuration);
    sb.Remove(configuration.IndexOf('}') - 1, 3);
    sb.Append(",\n  \"DataPlatformFilesPath\" : \"" + Application.persistentDataPath + "\"" +
    ", \n  \"DataPlatformCachePath\" : \"" + Application.temporaryCachePath + "\"" +
    ", \n  \"DataPlatformExternalPath\" : \"" + Application.temporaryCachePath + "\"" +
    ", \n  \"DataPlatformResourcesPath\" : \"" + PlatformUtil.GetResourcesFolder() + "\"" +
    ", \n  \"DataPlatformResourcesBasePath\" : \"" + PlatformUtil.GetResourcesBaseFolder() + "\"" +
    "\n}");

    return sb.ToString();
  }

  #region Mocks

  public void MockConnect() {

    _IsRobotConnected = true;

    const byte robotID = 1;

    Robots[robotID] = new MockRobot(robotID);
    CurrentRobotID = robotID;

    if (RobotConnected != null) {
      RobotConnected(robotID);
    }
  }

  #endregion //Mocks
}
