using Anki.Cozmo;
using UnityEngine;
using System.Collections;

public class ConnectionFlow : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  public const float kConnectionFlowDelay = 3.0f;

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
  private PullCubeTabView _PullCubeTabViewPrefab;
  private PullCubeTabView _PullCubeTabViewInstance;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private bool _Simulated = false;
  private string _CurrentRobotIP;

  private void Start() {
    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToEngine;
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(RobotConnectionResponse);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToEngine;
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(RobotConnectionResponse);

    Cleanup();
  }

  private void Cleanup() {
    if (_PullCubeTabViewInstance != null) {
      _PullCubeTabViewInstance.ViewClosed -= HandlePullCubeTabsCompeted;
      UIManager.CloseViewImmediately(_PullCubeTabViewInstance);
    }

    if (_ConnectionFlowBackgroundInstance != null) {
      GameObject.Destroy(_ConnectionFlowBackgroundInstance.gameObject);
    }

    if (_UpdateFirmwareScreenInstance != null) {
      GameObject.Destroy(_UpdateFirmwareScreenInstance.gameObject);
    }

    if (_InvalidPinViewInstance != null) {
      UIManager.CloseViewImmediately(_InvalidPinViewInstance);
    }

    if (_ReplaceCozmoOnChargerViewInstance != null) {
      UIManager.CloseViewImmediately(_ReplaceCozmoOnChargerViewInstance);
    }

    if (_WakingUpCozmoScreenInstance != null) {
      GameObject.Destroy(_WakingUpCozmoScreenInstance.gameObject);
    }
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
    if (_SearchForCozmoFailedScreenInstance != null) {
      GameObject.Destroy(_SearchForCozmoFailedScreenInstance.gameObject);
    }

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
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Cozmo_Connect);
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
    if (!success) {
      DAS.Warn("ConnectionFlow.FirmwareUpdated", "Firmware Update Failed");
    }
    else {
      DAS.Info("ConnectionFlow.FirmwareUpdated", "Firmware Update Successful");
    }

    GameObject.Destroy(_UpdateFirmwareScreenInstance.gameObject);
    ReplaceCozmoOnCharger();

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
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
        ShowPullCubeTabsFlow();
      }
      else {
        CheckForRestoreRobotFlow();
      }
    }
  }

  private void CheckForRestoreRobotFlow() {
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
      // we are done with first time user flow.. TODO: move this to after onboarding?
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow = false;
      DataPersistence.DataPersistenceManager.Instance.Save();

      // Need to create the stuct the will be written to the NVEntryTag, set values in it, and then pack it into a
      // memoryStream which will modify the byte array it was constructed with. That byte array is then what is
      // passed to the NVStorageWrite message
      Anki.Cozmo.OnboardingData data = new Anki.Cozmo.OnboardingData();
      data.hasCompletedOnboarding = true;
      byte[] byteArr = new byte[1024];
      System.IO.MemoryStream ms = new System.IO.MemoryStream(byteArr);
      data.Pack(ms);
      RobotEngineManager.Instance.CurrentRobot.NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_OnboardingData, (ushort)data.Size, byteArr);

      FinishConnectionFlow();
    }
    else {
      WakeupSequence();
    }
  }

  private void WakeupSequence() {
    _WakingUpCozmoScreenInstance = UIManager.CreateUIElement(_WakingUpCozmoScreenPrefab, _ConnectionFlowBackgroundInstance.transform);

    RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.ConnectWakeUp, HandleWakeAnimationComplete);
  }

  private void HandleWakeAnimationComplete(bool success) {
    GameObject.Destroy(_WakingUpCozmoScreenInstance);

    DAS.Debug("ConnectionFlow.HandleWakeAnimationComplete", "wake up animation: " + success);
    // explicitly enable charger behavior since it should be off by default in engine.

    FinishConnectionFlow();
  }

  private void ShowPullCubeTabsFlow() {
    _PullCubeTabViewInstance = UIManager.OpenView(_PullCubeTabViewPrefab);
    _PullCubeTabViewInstance.ViewClosed += HandlePullCubeTabsCompeted;
  }

  private void HandlePullCubeTabsCompeted() {
    CheckForRestoreRobotFlow();
  }

  private void FinishConnectionFlow() {
    UIManager.CloseView(_ConnectionFlowBackgroundInstance);
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("default_disabled", Anki.Cozmo.BehaviorType.ReactToOnCharger, true);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("wakeup", Anki.Cozmo.BehaviorType.AcknowledgeObject, true);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("wakeup", Anki.Cozmo.BehaviorType.ReactToCubeMoved, true);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("wakeup", Anki.Cozmo.BehaviorType.AcknowledgeFace, true);
    }

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
    // Silent if you've never done it before...
    if (!OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home)) {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Connecting);
    }
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

  private void RobotConnectionResponse(Anki.Cozmo.ExternalInterface.RobotConnectionResponse message) {
    // Set initial Robot Volume when connecting
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues(new Anki.Cozmo.Audio.VolumeParameters.VolumeType[] { Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot });
    DAS.Info("ConnectionFlow.RobotConnectionResponse", message.result.ToString());

    HandleRobotConnectResponse(message.result);
  }

  private void HandleRobotConnectResponse(RobotConnectionResult response) {
    switch (response) {
    case RobotConnectionResult.Success:
      RobotConnectResponseSuccess();
      break;
    case RobotConnectionResult.ConnectionFailure:
      if (_ConnectingToCozmoScreenInstance != null) {
        GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
      }
      ReturnToSearch();
      break;
    case RobotConnectionResult.OutdatedApp:
      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SkipFirmwareAutoUpdate) {
        RobotConnectResponseSuccess();
      }
      else {
        UpdateAppScreen();
      }
      break;
    case RobotConnectionResult.OutdatedFirmware:
      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SkipFirmwareAutoUpdate) {
        RobotConnectResponseSuccess();
      }
      else {
        UpdateFirmware();
      }
      break;
    case RobotConnectionResult.NeedsPin:
      GameObject.Destroy(_ConnectingToCozmoScreenInstance);
      ShowPinScreen();
      break;
    case RobotConnectionResult.InvalidPin:
      GameObject.Destroy(_ConnectingToCozmoScreenInstance);
      ShowInvalidPinScreen();
      break;
    case RobotConnectionResult.PinMaxAttemptsReached:
      ReplaceCozmoOnCharger();
      break;
    }
  }

  private void RobotConnectResponseSuccess() {
    if (_ConnectingToCozmoScreenInstance != null) {
      // progress from the connection screen to the next part of the flow
      _ConnectingToCozmoScreenInstance.ConnectionComplete();
    }

    // When we are in the first time user flow, we enable the block pool when we get to the Pull Cube Tab screen
    if (!DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
      // Enable the automatic block pool
      Anki.Cozmo.ExternalInterface.BlockPoolEnabledMessage blockPoolEnabledMessage = new Anki.Cozmo.ExternalInterface.BlockPoolEnabledMessage();
      blockPoolEnabledMessage.enabled = true;
      RobotEngineManager.Instance.Message.BlockPoolEnabledMessage = blockPoolEnabledMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    //Disable reactionary behaviors during wakeup
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("wakeup", Anki.Cozmo.BehaviorType.AcknowledgeObject, false);
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("wakeup", Anki.Cozmo.BehaviorType.ReactToCubeMoved, false);
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("wakeup", Anki.Cozmo.BehaviorType.AcknowledgeFace, false);
  }

  public void HandleRobotDisconnect() {
    if (_ReplaceCozmoOnChargerViewInstance != null) {
      // don't try to go through the search flow if the replace cozmo on charger view is up.
      return;
    }

    Cleanup();
    CreateConnectionFlowBackground();
    ShowSearchForCozmo();
  }
}
