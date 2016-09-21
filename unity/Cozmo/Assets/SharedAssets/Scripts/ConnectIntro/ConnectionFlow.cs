using Anki.Cozmo;
using UnityEngine;
using System.Collections;

public class ConnectionFlow : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  public static float ConnectionFlowDelay = 3.0f;

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

  private string _CurrentRobotIP;

  private bool _scanLoopPlaying = false;

  private void Start() {
    Cozmo.PauseManager.Instance.gameObject.SetActive(false);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(RobotConnectionResponse);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(RobotConnectionResponse);

    Cleanup();
    if (Cozmo.PauseManager.Instance != null) {
      Cozmo.PauseManager.Instance.gameObject.SetActive(true);
    }
  }

  private void Cleanup() {
    if (_PullCubeTabViewInstance != null) {
      _PullCubeTabViewInstance.ViewClosed -= HandlePullCubeTabsCompeted;
      UIManager.CloseViewImmediately(_PullCubeTabViewInstance);
    }

    if (_ConnectionFlowBackgroundInstance != null) {
      UIManager.CloseViewImmediately(_ConnectionFlowBackgroundInstance);
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

  public void StartConnectionFlow() {

    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
      _CurrentRobotIP = RobotEngineManager.kSimRobotIP;
      ConnectionFlowDelay = 0.25f;
    }
    else {
      _CurrentRobotIP = RobotEngineManager.kRobotIP;
    }
    CreateConnectionFlowBackground();
    ShowSearchForCozmo();
  }

  private void ReturnToTitle() {
    if (_ConnectionFlowBackgroundInstance != null) {
      _ConnectionFlowBackgroundInstance.ViewClosed += QuitConnectionFlow;
      UIManager.CloseView(_ConnectionFlowBackgroundInstance);
      _ConnectionFlowBackgroundInstance = null;
    }
    else {
      QuitConnectionFlow();
    }
    // Always Stop Loop sounds when leaving view
    PlayScanLoopAudio(false);
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

    if (_SearchForCozmoScreenInstance != null) {
      DAS.Error("ConnectionFlow.SearchForCozmo", "Search screen still exist! Don't create duplicate search screens");
      return;
    }

    _SearchForCozmoScreenInstance = UIManager.CreateUIElement(_SearchForCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SearchForCozmoScreen>();
    _SearchForCozmoScreenInstance.Initialize(_PingStatus);
    _SearchForCozmoScreenInstance.OnScreenComplete += HandleSearchForCozmoScreenDone;
    // Start Scan loop sound for connection phase
    PlayScanLoopAudio(true);
  }

  private void HandleSearchForCozmoScreenDone(bool success) {
    GameObject.Destroy(_SearchForCozmoScreenInstance.gameObject);

    if (success || RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
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
      // Stop scan loop when failed to connect
      PlayScanLoopAudio(false);
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
    // Restart Scan loop sound
    PlayScanLoopAudio(true);
  }

  private void ShowConnectingToCozmoScreen() {
    _ConnectingToCozmoScreenInstance = UIManager.CreateUIElement(_ConnectingToCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<ConnectingToCozmoScreen>();
    _ConnectingToCozmoScreenInstance.ConnectionScreenComplete += HandleConnectingToCozmoScreenDone;
    _ConnectionFlowBackgroundInstance.SetStateInProgress(1);

    ConnectToRobot();
  }

  private void HandleConnectingToCozmoScreenDone() {
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
    ReturnToSearch();
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
    // Stop scan loop sound connection phase is completed
    PlayScanLoopAudio(false);

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
        UIManager.CloseView(_ConnectionFlowBackgroundInstance);
        _ConnectionFlowBackgroundInstance = null;
      }
      else {
        CheckForRestoreRobotFlow();
      }
    }
  }

  private void CheckForRestoreRobotFlow() {
    OnboardingManager.Instance.FirstTime = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow;
    // we've never connected before so still have setup
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
      // we are done with first time user flow..
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow = false;
      DataPersistence.DataPersistenceManager.Instance.Save();

      // Need to create the stuct the will be written to the NVEntryTag, set values in it, and then pack it into a
      // memoryStream which will modify the byte array it was constructed with. That byte array is then what is
      // passed to the NVStorageWrite message
      Anki.Cozmo.OnboardingData data = new Anki.Cozmo.OnboardingData();
      data.hasCompletedOnboarding = true;
      byte[] byteArr = new byte[data.Size];
      System.IO.MemoryStream ms = new System.IO.MemoryStream(byteArr);
      data.Pack(ms);
      RobotEngineManager.Instance.CurrentRobot.NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_OnboardingData, byteArr);

      FinishConnectionFlow();
    }
    // connected before but quit while the "special moments" were still happening so play again rather than normal wakeup
    else if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home)) {
      FinishConnectionFlow();
    }
    // normal flow
    else {
      WakeupSequence();
    }
  }

  private void WakeupSequence() {
    _WakingUpCozmoScreenInstance = UIManager.CreateUIElement(_WakingUpCozmoScreenPrefab, _ConnectionFlowBackgroundInstance.transform);

    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
      HandleWakeAnimationComplete(true);
    }
    else {
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.ConnectWakeUp, HandleWakeAnimationComplete);
    }

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
    if (_ConnectionFlowBackgroundInstance != null) {
      UIManager.CloseView(_ConnectionFlowBackgroundInstance);
      _ConnectionFlowBackgroundInstance = null;
    }

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
    DAS.Info("ConnectionFlow.ConnectToRobot", "Trying to connect to robot " + _CurrentRobotIP);
    RobotEngineManager.Instance.ConnectToRobot(kRobotID, _CurrentRobotIP);
  }

  private void Disconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    DAS.Error("ConnectionFlow.Disconnected", "Robot Disconnected");
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
      // Audio Note: Let scaning loop continue to play
      RobotConnectResponseSuccess();
      break;

    case RobotConnectionResult.ConnectionFailure:
      if (_ConnectingToCozmoScreenInstance != null) {
        GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
      }
      // Audio Note: Let scaning loop continue to play
      ReturnToSearch();
      break;

    case RobotConnectionResult.OutdatedApp:
      PlayScanLoopAudio(false);
      UpdateAppScreen();
      break;

    case RobotConnectionResult.OutdatedFirmware:
      PlayScanLoopAudio(false);
      UpdateFirmware();
      break;

    case RobotConnectionResult.NeedsPin:
      GameObject.Destroy(_ConnectingToCozmoScreenInstance);
      PlayScanLoopAudio(false);
      ShowPinScreen();
      break;

    case RobotConnectionResult.InvalidPin:
      GameObject.Destroy(_ConnectingToCozmoScreenInstance);
      PlayScanLoopAudio(false);
      ShowInvalidPinScreen();
      break;

    case RobotConnectionResult.PinMaxAttemptsReached:
      PlayScanLoopAudio(false);
      ReplaceCozmoOnCharger();
      break;
    }
  }

  private void RobotConnectResponseSuccess() {
    if (_ConnectingToCozmoScreenInstance != null) {
      // progress from the connection screen to the next part of the flow
      _ConnectingToCozmoScreenInstance.ConnectionComplete();
    }

    if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
      DataPersistence.DataPersistenceManager.Instance.StartNewSession();
    }

    DataPersistence.DataPersistenceManager.Instance.CurrentSession.HasConnectedToCozmo = true;

    // When we are in the first time user flow, we enable the block pool when we get to the Pull Cube Tab screen
    if (!DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
      // Enable the automatic block pool
      RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(true);
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

    // If we're showing the update app view, the user will need to get a new version, so don't do anything when
    // the robot disconnects
    if (_UpdateAppViewInstance != null) {
      return;
    }

    if (_UpdateFirmwareScreenInstance != null && _UpdateFirmwareScreenInstance.DoneUpdateDelayInProgress) {
      // if we are delaying the firmware screen to wait for robot reboot don't reset the flow during the
      // delay.
      return;
    }

    Cleanup();
    CreateConnectionFlowBackground();
    ShowSearchForCozmo();
  }

  // Debounce Audio Loop events
  private void PlayScanLoopAudio(bool play) {
    if (_scanLoopPlaying == play) {
      // Do Nothing
      return;
    }
    _scanLoopPlaying = play;
    Anki.Cozmo.Audio.GameEvent.Ui evt = _scanLoopPlaying ? Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect_Scan_Loop : Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect_Scan_Loop_Stop;
    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(evt);
  }
}
