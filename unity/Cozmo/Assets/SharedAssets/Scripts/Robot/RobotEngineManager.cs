using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Anki.Cozmo.Audio;
using RobotChannel = ChannelBase<RobotMessageIn, RobotMessageOut>;

/// <summary>
/// Robot engine manager lives on a GameObject(named MasterObject) in our Intro scene,
/// and handles launching, ticking, and messaging with the Cozmo Engine
/// </summary>
public class RobotEngineManager : MonoBehaviour {

  public const string kRobotIP = "172.31.1.1";
  public const string kEngineIP = "127.0.0.1";
  public const string kSimRobotIP = "127.0.0.1";

  public enum ConnectionType {
    Robot,
    Sim,
    Mock
  }

  public ConnectionType RobotConnectionType = ConnectionType.Robot;

  public static RobotEngineManager Instance = null;

  private CallbackManager _CallbackManager = new CallbackManager();

  public void AddCallback<T>(Action<T> callback) {
    _CallbackManager.AddCallback(callback);
  }

  public void RemoveCallback<T>(Action<T> callback) {
    _CallbackManager.RemoveCallback(callback);
  }

  public Cozmo.BlockPool.BlockPoolTracker BlockPoolTracker { get; private set; }

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

  public bool IsConnectedToEngine { get { return (_Channel != null && _Channel.IsConnected); } }

  private List<string> _RobotAnimationNames = new List<string>();

  [SerializeField]
  private TextAsset _Configuration;

  private DisconnectionReason _LastDisconnectionReason = DisconnectionReason.None;

  public event Action<string> ConnectedToClient;

  private bool _CozmoBindingStarted = false;
  private RobotChannel _Channel = null;
  private bool _IsRobotConnected = false;

  private const int _UIDeviceID = 1;
  private const int _UIAdvertisingRegistrationPort = 5103;
  private const int _UILocalPort = 0;

  private RobotMessageOut _MessageOut = new RobotMessageOut();

  public Anki.Cozmo.ExternalInterface.MessageGameToEngine Message { get { return _MessageOut.Message; } }

  private Anki.Cozmo.ExternalInterface.StartEngine StartEngineMessage = new Anki.Cozmo.ExternalInterface.StartEngine();
  private Anki.Cozmo.ExternalInterface.ConnectToRobot ConnectToRobotMessage = new Anki.Cozmo.ExternalInterface.ConnectToRobot();
  private Anki.Cozmo.ExternalInterface.ConnectToUiDevice ConnectToUiDeviceMessage = new Anki.Cozmo.ExternalInterface.ConnectToUiDevice(UiConnectionType.UI, 0);

  private Anki.Cozmo.ExternalInterface.GetAllDebugConsoleVarMessage _GetAllDebugConsoleVarMessage = new Anki.Cozmo.ExternalInterface.GetAllDebugConsoleVarMessage();
  private Anki.Cozmo.ExternalInterface.SetDebugConsoleVarMessage _SetDebugConsoleVarMessage = new Anki.Cozmo.ExternalInterface.SetDebugConsoleVarMessage();
  private Anki.Cozmo.ExternalInterface.RunDebugConsoleFuncMessage _RunDebugConsoleFuncMessage = new Anki.Cozmo.ExternalInterface.RunDebugConsoleFuncMessage();
  private Anki.Cozmo.ExternalInterface.DenyGameStart _DenyGameStartMessage = new Anki.Cozmo.ExternalInterface.DenyGameStart();
  private Anki.Cozmo.ExternalInterface.ResetFirmware _ResetFirmwareMessage = new Anki.Cozmo.ExternalInterface.ResetFirmware();
  private Anki.Cozmo.ExternalInterface.EnableReactionaryBehaviors _EnableReactionaryBehaviorMessage = new Anki.Cozmo.ExternalInterface.EnableReactionaryBehaviors();
  private Anki.Cozmo.ExternalInterface.RequestDeviceData _RequestDeviceDataMessage = new Anki.Cozmo.ExternalInterface.RequestDeviceData();
  private Anki.Cozmo.ExternalInterface.RequestUnlockDataFromBackup _RequestUnlockDataFromBackupMessage = new Anki.Cozmo.ExternalInterface.RequestUnlockDataFromBackup();

  private void OnEnable() {
    DAS.Event("RobotEngineManager.OnEnable", string.Empty);
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
    }

    Application.runInBackground = true;

#if !UNITY_EDITOR && (UNITY_IOS || UNITY_ANDROID)
    _Channel = new RobotDirectChannel();
#else
    _Channel = new RobotUdpChannel();
#endif
    _Channel.ConnectedToClient += Connected;
    _Channel.DisconnectedFromClient += Disconnected;
    _Channel.MessageReceived += ReceivedMessage;

    Robots = new Dictionary<int, IRobot>();

    FeatureGate.Instance.Initialize();
    Anki.Debug.DebugConsoleData.Instance.RegisterEngineCallbacks();
  }

  private void OnDisable() {

    if (_Channel != null) {
      if (_Channel.IsActive) {
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

  public void CozmoEngineInitialization() {

    TextAsset config = _Configuration;

    if (config == null) {
      DAS.Error("RobotEngineManager.ErrorInitializingCozmoBinding.NoConfig", string.Empty);
    }
    else {
      JSONObject configJson = JSONObject.Create(config.text);
      AddDataPlatformPathsToConfiguration(configJson);

      CozmoBinding.Startup(configJson);
      _CozmoBindingStarted = true;
    }

  }

  public List<string> GetRobotAnimationNames() {
    return _RobotAnimationNames;
  }

  public void SetEnableSOSLogging(bool enable) {
    DAS.Debug(this, "Set enable SOS Logging: " + enable);
    RobotEngineManager.Instance.Message.SetEnableSOSLogging = Singleton<SetEnableSOSLogging>.Instance.Initialize(enable);
    RobotEngineManager.Instance.SendMessage();
  }

  public void AddRobot(byte robotID) {
    RemoveRobot(robotID);

    IRobot robot = new Robot(robotID);
    Robots.Add(robotID, robot);
    CurrentRobotID = robotID;

    if (BlockPoolTracker != null) {
      BlockPoolTracker.InitBlockPool();
    }
  }

  public void RemoveRobot(byte robotID) {
    IRobot oldRobot;
    if (Robots.TryGetValue(robotID, out oldRobot)) {
      if (oldRobot != null) {
        oldRobot.Dispose();
      }
      _LastCurrentRobot = null;
      Robots.Remove(robotID);

      if (0 == Robots.Count) {
        _IsRobotConnected = false;
      }

      if (BlockPoolTracker != null) {
        BlockPoolTracker.SendAvailableObjects(false, (byte)robotID);
      }
    }
  }

  public void Connect(string engineIP) {
    DAS.Info("RobotEngineManager.Connect", "Unity attempting to connect to engine");
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

    if (BlockPoolTracker == null) {
      BlockPoolTracker = new Cozmo.BlockPool.BlockPoolTracker(this);
    }
  }

  private void Disconnected(DisconnectionReason reason) {
    DAS.Debug("RobotEngineManager.Disconnected", reason.ToString());
    Disconnect();
  }

  public void SendMessage() {
    if (!IsConnectedToEngine) {
      DAS.Warn("RobotEngineManager.MessageNotSent", "Not Connected");
      return;
    }

    _Channel.Send(_MessageOut);
  }

  private void ReceivedMessage(RobotMessageIn messageIn) {
    var message = messageIn.Message;

    switch (message.GetTag()) {
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.Ping:
      ProcessPingResponse(message.Ping);
      break;
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.UiDeviceAvailable:
      ProcessUiDeviceAvailable(message.UiDeviceAvailable);
      break;
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.RobotConnectionResponse:
      ProcessRobotConnectionResponse(message.RobotConnectionResponse);
      break;
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.RobotDisconnected:
      ProcessRobotDisconnected(message.RobotDisconnected);
      break;
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.EngineRobotCLADVersionMismatch:
      ProcessCLADVersionMismatch(message.EngineRobotCLADVersionMismatch);
      break;
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.AnimationAvailable:
      SetAvailableAnimationNames(message.AnimationAvailable);
      break;
    case Anki.Cozmo.ExternalInterface.MessageEngineToGame.Tag.SupportInfo:
      ProcessSupportInfo(message.SupportInfo);
      break;
    }

    // since the property to access individual message data in a CLAD message shares its name
    // with the message's tag, we can use that name to get the property that retrieves message
    // data for this type, and execute it on this message
    object messageData = typeof(Anki.Cozmo.ExternalInterface.MessageEngineToGame).GetProperty(message.GetTag().ToString()).GetValue(message, null);
    // callback manager will use the runtime type of messageData to notify subscribers of the
    // incoming message
    _CallbackManager.MessageReceived(messageData);
  }

  public void MockCallback(MessageEngineToGame message, float delay = 0.0f) {
    StartCoroutine(MockDelayedCallback(message, delay));
  }

  private IEnumerator MockDelayedCallback(MessageEngineToGame message, float delay) {
    yield return new WaitForSeconds(delay);
    object messageData = typeof(Anki.Cozmo.ExternalInterface.MessageEngineToGame).GetProperty(message.GetTag().ToString()).GetValue(message, null);
    _CallbackManager.MessageReceived(messageData);
  }

  private void ProcessPingResponse(Anki.Cozmo.ExternalInterface.Ping message) {
    if (message.isResponse) {
      DAS.Warn("Unity.ReceivedResponsePing", "Unity receiving response pings is unsupported");
    }
    else {
      Message.Ping = Singleton<Anki.Cozmo.ExternalInterface.Ping>.Instance.Initialize(counter: message.counter, timeSent_ms: message.timeSent_ms, isResponse: true);
      SendMessage();
    }
  }

  private void ProcessUiDeviceAvailable(Anki.Cozmo.ExternalInterface.UiDeviceAvailable message) {
    ConnectToUiDeviceMessage.deviceID = (byte)message.deviceID;

    Message.ConnectToUiDevice = ConnectToUiDeviceMessage;
    SendMessage();
  }

  private void ProcessRobotConnectionResponse(Anki.Cozmo.ExternalInterface.RobotConnectionResponse message) {
    // Right now we only add one robot. in the future this should be
    // expanded potentially add multiple robots.
    if (!_IsRobotConnected && message.result != RobotConnectionResult.ConnectionFailure) {
      _IsRobotConnected = true;
      AddRobot((byte)message.robotID);
      CurrentRobot.FirmwareVersion = message.fwVersion;
      CurrentRobot.SerialNumber = message.serialNumber;

      var persistence = DataPersistence.DataPersistenceManager.Instance;
      var currentSerial = persistence.Data.DeviceSettings.LastCozmoSerial;
      if (message.serialNumber != currentSerial) {
        persistence.Data.DeviceSettings.LastCozmoSerial = message.serialNumber;
        persistence.Save();
      }

      PlayTimeManager.Instance.RobotConnected(true);
      DasTracker.Instance.TrackRobotConnected();
    }
  }

  private void ProcessRobotDisconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    DasTracker.Instance.TrackRobotDisconnected((byte)message.robotID);
    DAS.Error("RobotEngineManager.RobotDisconnected", "Robot " + message.robotID + " disconnected after " + message.timeSinceLastMsg_sec.ToString("0.00") + " seconds.");
    RemoveRobot((byte)message.robotID);
    PlayTimeManager.Instance.RobotConnected(false);
  }

  private void ProcessCLADVersionMismatch(Anki.Cozmo.ExternalInterface.EngineRobotCLADVersionMismatch message) {
    if (message.engineToRobotMismatch) {
      string str = "Engine to Robot CLAD version mismatch. Engine's EngineToRobot hash = " + message.engineEnginetoRobotHash + ". Robot's EngineToRobot hash = " + message.robotEnginetoRobotHash;
      DAS.Debug("clad_version_mismatch_engine_to_robot", str);
    }
    if (message.robotToEngineMistmatch) {
      string str = "Robot to Engine CLAD version mismatch. Engine's EngineToRobot hash = " + message.engineRobotToEngineHash + ". Robot's RobotToEngine hash = " + message.robotRobotToEngineHash;
      DAS.Debug("clad_version_mismatch_robot_to_engine", str);
    }
  }

  private void SetAvailableAnimationNames(Anki.Cozmo.ExternalInterface.AnimationAvailable message) {
    if (!_RobotAnimationNames.Contains(message.animName))
      _RobotAnimationNames.Add(message.animName);
  }

  private void ProcessSupportInfo(Anki.Cozmo.ExternalInterface.SupportInfo message) {
    DataPersistence.DataPersistenceManager.Instance.HandleSupportInfo(message);
  }

  public void StartEngine() {
    Message.StartEngine = StartEngineMessage;
    SendMessage();
  }

  public void UpdateFirmware(Anki.Cozmo.FirmwareType type, int firmwareVersion) {
    Message.UpdateFirmware = Singleton<Anki.Cozmo.ExternalInterface.UpdateFirmware>.Instance.Initialize(type, firmwareVersion);
    SendMessage();
  }

  public void ResetFirmware() {
    Message.ResetFirmware = _ResetFirmwareMessage;
    SendMessage();
  }

  public void TurnOffReactionaryBehavior() {
    if (CurrentRobot != null) {
      CurrentRobot.ResetRobotState();
    }

    SetEnableReactionaryBehaviors(false);
  }

  public void SetEnableReactionaryBehaviors(bool enable) {
    _EnableReactionaryBehaviorMessage.enabled = enable;
    Message.EnableReactionaryBehaviors = _EnableReactionaryBehaviorMessage;
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

  public void SendRequestDeviceData() {
    Message.RequestDeviceData = _RequestDeviceDataMessage;
    SendMessage();
  }

  public void SendRequestUnlockDataFromBackup() {
    Message.RequestUnlockDataFromBackup = _RequestUnlockDataFromBackupMessage;
    SendMessage();
  }

  public void ConnectToRobot(int robotID, string robotIP) {
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
    ConnectToRobotMessage.isSimulated = RobotConnectionType == ConnectionType.Sim ? (byte)1 : (byte)0;

    Message.ConnectToRobot = ConnectToRobotMessage;
    SendMessage();
  }

  private void AddDataPlatformPathsToConfiguration(JSONObject json) {
    json.AddField("DataPlatformFilesPath", Application.persistentDataPath);
    json.AddField("DataPlatformCachePath", Application.temporaryCachePath);
    json.AddField("DataPlatformExternalPath", Application.temporaryCachePath);
    json.AddField("DataPlatformResourcesPath", PlatformUtil.GetResourcesFolder());
    json.AddField("DataPlatformResourcesBasePath", PlatformUtil.GetResourcesBaseFolder());
  }

  public void StartIdleTimeout(float faceOffTime_s, float disconnectTime_s) {
    Message.StartIdleTimeout = Singleton<Anki.Cozmo.ExternalInterface.StartIdleTimeout>.Instance.Initialize(faceOffTime_s, disconnectTime_s);
    SendMessage();
  }

  public void CancelIdleTimeout() {
    Message.CancelIdleTimeout = Singleton<Anki.Cozmo.ExternalInterface.CancelIdleTimeout>.Instance;
    SendMessage();
  }

  public void FlushChannelMessages() {
    _Channel.FlushQueuedMessages();
  }

  public void SendGameBeingPaused(bool isPaused) {
    Message.SetGameBeingPaused = Singleton<Anki.Cozmo.ExternalInterface.SetGameBeingPaused>.Instance.Initialize(isPaused);
    SendMessage();
  }

  #region Mocks

  public void MockConnect() {

    _IsRobotConnected = true;

    const byte robotID = 1;

    Robots[robotID] = new MockRobot(robotID);
    CurrentRobotID = robotID;
    if (DataPersistence.DataPersistenceManager.Instance.CurrentSession != null) {
      DataPersistence.DataPersistenceManager.Instance.CurrentSession.HasConnectedToCozmo = true;
    }

    // mock connect message fire.
    Anki.Cozmo.ExternalInterface.RobotConnectionResponse connectedMessage = new Anki.Cozmo.ExternalInterface.RobotConnectionResponse();
    connectedMessage.robotID = 1;
    connectedMessage.result = RobotConnectionResult.Success;
    _CallbackManager.MessageReceived(connectedMessage);
  }

  #endregion //Mocks
}
