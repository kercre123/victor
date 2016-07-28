using UnityEngine;
using System.Collections;

public class ConnectionFlow : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  public const float kConnectionFlowDelay = 1.0f;

  [SerializeField]
  private ConnectionFlowBackground _ConnectionFlowBackgroundPrefab;
  private ConnectionFlowBackground _ConnectionFlowBackgroundInstance;

  [SerializeField]
  private SearchForCozmoScreen _SearchForCozmoScreenPrefab;
  private SearchForCozmoScreen _SearchForCozmoScreenInstance;

  [SerializeField]
  private SearchForCozmoFailedScreen _SearchForCozmoFailedScreenPrefab;
  private SearchForCozmoFailedScreen _SearchForCozmoFailedScreenInstance;

  [SerializeField]
  private SecuringConnectionScreen _SecuringConnectionScreenPrefab;
  private SecuringConnectionScreen _SecuringConnectionScreenInstance;

  [SerializeField]
  private ConnectingToCozmoScreen _ConnectingToCozmoScreenPrefab;
  private ConnectingToCozmoScreen _ConnectingToCozmoScreenInstance;

  [SerializeField]
  private UpdateFirmwareScreen _UpdateFirmwareScreenPrefab;
  private UpdateFirmwareScreen _UpdateFirmwareScreenInstance;

  [SerializeField]
  private UpdateAppView _UpdateAppViewPrefab;
  private UpdateAppView _UpdateAppViewInstance;

  [SerializeField]
  private GameObject _WakingUpCozmoScreenPrefab;
  private GameObject _WakingUpCozmoScreenInstance;

  [SerializeField]
  private PinSecurityView _PinSecurityViewPrefab;
  private PinSecurityView _PinSecurityViewInstance;

  [SerializeField]
  private InvalidPinView _InvalidPinViewPrefab;
  private InvalidPinView _InvalidPinViewInstance;

  [SerializeField]
  private SimpleConnectView _ReplaceCozmoOnChargerViewPrefab;
  private SimpleConnectView _ReplaceCozmoOnChargerViewInstance;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private bool _Simulated = false;
  private string _CurrentRobotIP;

  private void Start() {
    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToEngine;
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(RobotConnectionResponse);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToEngine;
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(RobotConnectionResponse);
  }

  public void Play(bool sim) {
    _Simulated = sim;

    if (sim) {
      _CurrentRobotIP = RobotEngineManager.kSimRobotIP;
    }
    else {
      _CurrentRobotIP = RobotEngineManager.kRobotIP;
    }

    CreateConnectionFlowBackground();
    ShowSearchForCozmo();
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Connecting);
  }

  private void ReturnToTitle() {
    _ConnectionFlowBackgroundInstance.ViewClosed += QuitConnectionFlow;
    UIManager.CloseView(_ConnectionFlowBackgroundInstance);
  }

  private void QuitConnectionFlow() {
    if (ConnectionFlowQuit != null) {
      ConnectionFlowQuit();
    }
  }

  private void ReturnToSearch() {
    _ConnectionFlowBackgroundInstance.ResetAllProgress();
    ShowSearchForCozmo();
  }

  private void CreateConnectionFlowBackground() {
    _ConnectionFlowBackgroundInstance = UIManager.OpenView<ConnectionFlowBackground>(_ConnectionFlowBackgroundPrefab);
  }

  private void ShowSearchForCozmo() {
    _ConnectionFlowBackgroundInstance.SetStateInProgress(0);
    _SearchForCozmoScreenInstance = UIManager.CreateUIElement(_SearchForCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SearchForCozmoScreen>();
    _SearchForCozmoScreenInstance.Initialize(_PingStatus);
    _SearchForCozmoScreenInstance.OnScreenComplete += HandleSearchForCozmoScreenDone;
  }

  private void HandleSearchForCozmoScreenDone(bool success) {
    GameObject.Destroy(_SearchForCozmoScreenInstance.gameObject);

    if (success || _Simulated) {
      _ConnectionFlowBackgroundInstance.SetStateComplete(0);
      ShowConnectingToCozmoScreen();
    }
    else {
      // TODO: do BLE search before showing wifi screens
      _SearchForCozmoFailedScreenInstance = UIManager.CreateUIElement(_SearchForCozmoFailedScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SearchForCozmoFailedScreen>();
      _SearchForCozmoFailedScreenInstance.OnEndpointFound += HandleEndpointFound;
      _SearchForCozmoFailedScreenInstance.OnQuitFlow += HandleOnQuitFlowFromFailedSearch;
      _SearchForCozmoFailedScreenInstance.Initialize(_PingStatus);
      _ConnectionFlowBackgroundInstance.SetStateFailed(0);
    }
  }

  private void HandleOnQuitFlowFromFailedSearch() {
    GameObject.Destroy(_SearchForCozmoFailedScreenInstance.gameObject);
    ReturnToTitle();
  }

  private void HandleEndpointFound() {
    _SearchForCozmoFailedScreenInstance.OnEndpointFound -= HandleEndpointFound;
    _SearchForCozmoFailedScreenInstance.OnQuitFlow -= HandleOnQuitFlowFromFailedSearch;
    GameObject.Destroy(_SearchForCozmoFailedScreenInstance.gameObject);
    _ConnectionFlowBackgroundInstance.SetStateComplete(0);
    ShowConnectingToCozmoScreen();
  }

  private void ShowConnectingToCozmoScreen() {
    _ConnectingToCozmoScreenInstance = UIManager.CreateUIElement(_ConnectingToCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<ConnectingToCozmoScreen>();
    _ConnectingToCozmoScreenInstance.ConnectionScreenComplete += HandleConnectingToCozmoScreenDone;
    _ConnectionFlowBackgroundInstance.SetStateInProgress(1);

    TryConnect();
  }

  private void TryConnect() {
    // In editor we delay the connection to the engine. This is so we can use things
    // like mock mode which does not require the engine.
    if (RobotEngineManager.Instance.IsConnectedToEngine) {
      ConnectToRobot();
    }
    else {
      ConnectToEngine();
    }
  }

  private void HandleConnectingToCozmoScreenDone() {
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.Cozmo_Connect);
    GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
    _ConnectionFlowBackgroundInstance.SetStateComplete(1);
    ShowSecuringConnectionScreen();
  }

  private void UpdateFirmware() {
    GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
    _UpdateFirmwareScreenInstance = UIManager.CreateUIElement(_UpdateFirmwareScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<UpdateFirmwareScreen>();
    _UpdateFirmwareScreenInstance.FirmwareUpdateDone += FirmwareUpdated;
  }

  private void FirmwareUpdated(bool success) {
    GameObject.Destroy(_UpdateFirmwareScreenInstance.gameObject);

    if (success) {
      ReturnToSearch();
    }
    else {
      ReplaceCozmoOnCharger();
    }

  }

  private void ReplaceCozmoOnCharger() {
    _ReplaceCozmoOnChargerViewInstance = UIManager.OpenView(_ReplaceCozmoOnChargerViewPrefab);
    _ReplaceCozmoOnChargerViewInstance.OnConnectButton += ReplaceCozmoOnChargerConnect;
    _ReplaceCozmoOnChargerViewInstance.ViewClosedByUser += ReturnToTitle;
  }

  private void ReplaceCozmoOnChargerConnect() {
    UIManager.CloseView(_ReplaceCozmoOnChargerViewInstance);
    ReturnToSearch();
  }

  private void ShowSecuringConnectionScreen() {
    _ConnectionFlowBackgroundInstance.SetStateInProgress(2);
    _SecuringConnectionScreenInstance = UIManager.CreateUIElement(_SecuringConnectionScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SecuringConnectionScreen>();
    _SecuringConnectionScreenInstance.OnScreenComplete += HandleSecuringConnectionScreenDone;
  }

  private void ShowInvalidPinScreen() {
    _InvalidPinViewInstance = UIManager.OpenView(_InvalidPinViewPrefab);
    _InvalidPinViewInstance.OnRetryPin += ShowPinScreen;
    _InvalidPinViewInstance.ViewClosedByUser += ReturnToTitle;
  }

  private void ShowPinScreen() {
    _PinSecurityViewInstance = UIManager.OpenView(_PinSecurityViewPrefab);
    _PinSecurityViewInstance.ViewClosedByUser += HandlePinSecurityViewClosedByUser;
    _PinSecurityViewInstance.OnPinEntered += HandlePinSecurityEntered;
  }

  private void UpdateAppScreen() {
    _UpdateAppViewInstance = UIManager.OpenView(_UpdateAppViewPrefab);
    _UpdateAppViewInstance.ViewClosedByUser += ReturnToTitle;
  }

  private void HandlePinSecurityViewClosedByUser() {
    ReturnToTitle();
  }

  private void HandlePinSecurityEntered(string pin) {
    UIManager.CloseView(_PinSecurityViewInstance);
    ShowConnectingToCozmoScreen();
  }

  private void HandleSecuringConnectionScreenDone() {
    _SecuringConnectionScreenInstance.OnScreenComplete -= HandleSecuringConnectionScreenDone;
    GameObject.Destroy(_SecuringConnectionScreenInstance.gameObject);

    _ConnectionFlowBackgroundInstance.SetStateComplete(2);

    // press demo skips the wake up seqeuence since it has its own flow
    // for snoring/wake-up.
    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
      HandleWakeAnimationComplete(true);
    }
    else {
      _WakingUpCozmoScreenInstance = UIManager.CreateUIElement(_WakingUpCozmoScreenPrefab, _ConnectionFlowBackgroundInstance.transform);
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.ConnectWakeUp, HandleWakeAnimationComplete);
    }

  }

  private void HandleWakeAnimationComplete(bool success) {
    GameObject.Destroy(_WakingUpCozmoScreenInstance);
    UIManager.CloseView(_ConnectionFlowBackgroundInstance);

    // explicitly enable charger behavior since it should be off by default in engine.
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("on_start_wakeup", Anki.Cozmo.BehaviorType.ReactToOnCharger, true);
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
  }

  private void ConnectToEngine() {
    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.Connect(RobotEngineManager.kEngineIP);
  }

  private void ConnectToRobot() {
    DAS.Info("ConnectionFlow.ConnectToRobot", "Trying to connect to robot");
    RobotEngineManager.Instance.ConnectToRobot(kRobotID, _CurrentRobotIP, _Simulated);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Connecting);
  }

  private void HandleConnectedToEngine(string connectionIdentifier) {
#if UNITY_EDITOR
    // in editor we have to wait to connect to the engine first before connecting
    // to the robot.
    DAS.Info("ConnectionFlow.HandleConnectedToEngine", "In Editor so connecting to robot after connecting to engine");
    SetupEngine();
    ConnectToRobot();
#endif
  }

  private void Disconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    DAS.Error("ConnectionFlow.Disconnected", "Robot Disconnected");
  }

  private void SetupEngine() {
    RobotEngineManager.Instance.StartEngine();
    // Set initial volumes
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
  }

  private void RobotConnectionResponse(Anki.Cozmo.ExternalInterface.RobotConnected message) {
    int robotID = (int)message.robotID;
    if (!RobotEngineManager.Instance.Robots.ContainsKey(robotID)) {
      DAS.Error(this, "Unknown robot connected: " + robotID.ToString());
      return;
    }

    DAS.Debug("ConnectionFlow.RobotConnectionResponse", "RobotID: " + robotID);

    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled) {
      ConsoleLogManager.Instance.EnableSOSLogs(true);
    }

    // Set initial Robot Volume when connecting
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues(new Anki.Cozmo.Audio.VolumeParameters.VolumeType[] { Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot });
    DAS.Info("ConnectionFlow.RobotConnectionResponse", "Robot Connect Request Responded!");
    // TODO: replace with actual robot response from engine.
    HandleRobotConnectResponse(RobotConnectResponse.Success);
  }

  private void HandleRobotConnectResponse(RobotConnectResponse response) {
    switch (response) {
    case RobotConnectResponse.Success:
      if (_ConnectingToCozmoScreenInstance != null) {
        _ConnectingToCozmoScreenInstance.ConnectionComplete();
      }
      break;
    case RobotConnectResponse.Failed:
      if (_ConnectingToCozmoScreenInstance != null) {
        GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
      }
      ReturnToSearch();
      break;
    case RobotConnectResponse.NeedsAppUpgrade:
      UpdateAppScreen();
      break;
    case RobotConnectResponse.NeedsFirmwareUpgrade:
      UpdateFirmware();
      break;
    case RobotConnectResponse.NeedsPin:
      GameObject.Destroy(_ConnectingToCozmoScreenInstance);
      ShowPinScreen();
      break;
    case RobotConnectResponse.InvalidPin:
      GameObject.Destroy(_ConnectingToCozmoScreenInstance);
      ShowInvalidPinScreen();
      break;
    case RobotConnectResponse.PinMaxAttemptReached:
      ReplaceCozmoOnCharger();
      break;
    }
  }
}
