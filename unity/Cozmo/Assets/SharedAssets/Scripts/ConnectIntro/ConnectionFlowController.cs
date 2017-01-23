using Anki.Cozmo;
using UnityEngine;
using System;
using System.Collections;
using System.Linq;
using Cozmo.UI;

public class ConnectionFlowController : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  public static float ConnectionFlowDelay = 3.0f;

  [SerializeField]
  private ConnectionFlowBackgroundModal _ConnectionFlowBackgroundModalPrefab;
  private ConnectionFlowBackgroundModal _ConnectionFlowBackgroundModalInstance;

  [SerializeField]
  private SearchForCozmoScreen _SearchForCozmoScreenPrefab;
  [SerializeField]
  private AndroidConnectionFlow _AndroidConnectionFlowPrefab;

  private GameObject _SearchForCozmoScreenInstance;

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
  private ConnectionRejectedScreen _ConnectionRejectedScreenPrefab;
  private ConnectionRejectedScreen _ConnectionRejectedScreenInstance;

  [SerializeField]
  private UpdateFirmwareScreen _UpdateFirmwareScreenPrefab;
  private UpdateFirmwareScreen _UpdateFirmwareScreenInstance;

  [SerializeField]
  private UpdateAppModal _UpdateAppModalPrefab;
  private UpdateAppModal _UpdateAppModalInstance;

  [SerializeField]
  private GameObject _WakingUpCozmoScreenPrefab;
  private GameObject _WakingUpCozmoScreenInstance;

  [SerializeField]
  private PinSecurityModal _PinSecurityModalPrefab;
  private PinSecurityModal _PinSecurityModalInstance;

  [SerializeField]
  private InvalidPinModal _InvalidPinModalPrefab;
  private InvalidPinModal _InvalidPinModalInstance;

  [SerializeField]
  private SimpleConnectModal _ReplaceCozmoOnChargerModalPrefab;
  private SimpleConnectModal _ReplaceCozmoOnChargerModalInstance;

  [SerializeField]
  private PullCubeTabModal _PullCubeTabModalPrefab;
  private PullCubeTabModal _PullCubeTabModalInstance;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private string _CurrentRobotIP;

  private bool _scanLoopPlaying = false;

  private void Awake() {
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

  private void OnApplicationPause(bool bPause) {
    // manually tell engine we are paused/unpaused because we disabled pause manager.
    RobotEngineManager.Instance.SendGameBeingPaused(bPause);
    // flush the message to make sure it is sent
    RobotEngineManager.Instance.FlushChannelMessages();
  }

  private void Cleanup() {
    if (_PullCubeTabModalInstance != null) {
      _PullCubeTabModalInstance.DialogClosed -= HandlePullCubeTabsCompleted;
      UIManager.CloseModalImmediately(_PullCubeTabModalInstance);
    }

    if (_ConnectionFlowBackgroundModalInstance != null) {
      UIManager.CloseModalImmediately(_ConnectionFlowBackgroundModalInstance);
    }

    if (_UpdateFirmwareScreenInstance != null) {
      GameObject.Destroy(_UpdateFirmwareScreenInstance.gameObject);
    }

    if (_InvalidPinModalInstance != null) {
      UIManager.CloseModalImmediately(_InvalidPinModalInstance);
    }

    if (_ReplaceCozmoOnChargerModalInstance != null) {
      UIManager.CloseModalImmediately(_ReplaceCozmoOnChargerModalInstance);
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
    InitConnectionFlow();
  }

  private void ReturnToTitle() {
    if (_ConnectionFlowBackgroundModalInstance != null) {
      _ConnectionFlowBackgroundModalInstance.DialogClosed += QuitConnectionFlow;
      UIManager.CloseModal(_ConnectionFlowBackgroundModalInstance);
      _ConnectionFlowBackgroundModalInstance = null;
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
    _ConnectionFlowBackgroundModalInstance.ResetAllProgress();
    ShowSearchForCozmo();
  }

  private void InitConnectionFlow() {
    if (FeatureGate.Instance.IsFeatureEnabled(FeatureType.AndroidConnectionFlow)) {
      #if UNITY_ANDROID && !UNITY_EDITOR
      if (AndroidConnectionFlow.IsAvailable() && !AndroidConnectionFlow.HandleAlreadyOnCozmoWifi()) {
        ShowSearchForCozmoAndroid();
        return;
      }
      #endif
    }
    // fall-thru from not selecting android flow
    CreateConnectionFlowBackground();
  }

  private void CreateConnectionFlowBackground() {
    CreateConnectionFlowBackgroundWithCallback(ShowSearchForCozmo);
  }

  private void CreateConnectionFlowBackgroundWithCallback(Action onComplete) {
    UIManager.OpenModal(_ConnectionFlowBackgroundModalPrefab, new ModalPriorityData(), (arg) => HandleConnectionFlowBackgroundCreationFinished(arg, onComplete));
  }

  private void HandleConnectionFlowBackgroundCreationFinished(BaseModal newConnectionFlowBackground, Action launchAction) {
    _ConnectionFlowBackgroundModalInstance = (ConnectionFlowBackgroundModal)newConnectionFlowBackground;
    launchAction();
  }

  private void ShowSearchForCozmoAndroid() {
    DAS.Info("ConnectionFlow.ShowAndroid", "Instantiating android flow");
    var androidFlowInstance = GameObject.Instantiate(_AndroidConnectionFlowPrefab.gameObject).GetComponent<AndroidConnectionFlow>();
    _SearchForCozmoScreenInstance = androidFlowInstance.gameObject;

    // Set up event handlers for things the Android flow might tell us:
    // - if we want to start the flow over again
    androidFlowInstance.OnRestartFlow += () => {
      // explicitly unregister handlers before instantiating a new instance,
      // since OnDestroy() invocation is delayed until after frame
      androidFlowInstance.Disable();
      GameObject.Destroy(androidFlowInstance.gameObject);
      ShowSearchForCozmoAndroid();
    };
    // - when the screen finishes, either continue old connection flow accordingly
    //   or go to "search failed" screen
    Action<bool, string> onCompleteFunc = (success, stringName) => {
      DAS.Info("ConnectionFlow.ShowAndroid", stringName);
      CreateConnectionFlowBackgroundWithCallback(() => {
        _ConnectionFlowBackgroundModalInstance.SetStateInProgress(0);
        HandleSearchForCozmoScreenDone(success);
      });
    };
    androidFlowInstance.OnScreenComplete += (success) => onCompleteFunc(success, "OnComplete");
    // - if the old flow is requested, start it
    androidFlowInstance.OnCancelFlow += () => onCompleteFunc(false, "CancelAndroid");
  }

  private void ShowSearchForCozmo() {
    _ConnectionFlowBackgroundModalInstance.SetStateInProgress(0);

    if (_SearchForCozmoScreenInstance != null) {
      DAS.Error("ConnectionFlow.SearchForCozmo", "Search screen still exist! Don't create duplicate search screens");
      return;
    }

    var screenInstance = UIManager.CreateUIElement(_SearchForCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<SearchForCozmoScreen>();
    screenInstance.Initialize(_PingStatus);
    screenInstance.OnScreenComplete += HandleSearchForCozmoScreenDone;
    _SearchForCozmoScreenInstance = screenInstance.gameObject;
    // Start Scan loop sound for connection phase
    PlayScanLoopAudio(true);
  }

  private void HandleSearchForCozmoScreenDone(bool success) {
    GameObject.Destroy(_SearchForCozmoScreenInstance);

    if (success || RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
      _ConnectionFlowBackgroundModalInstance.SetStateComplete(0);
      ShowConnectingToCozmoScreen();
    }
    else {
      _SearchForCozmoFailedScreenInstance = UIManager.CreateUIElement(_SearchForCozmoFailedScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<SearchForCozmoFailedScreen>();
      _SearchForCozmoFailedScreenInstance.OnEndpointFound += HandleEndpointFound;
      _SearchForCozmoFailedScreenInstance.OnQuitFlow += HandleOnQuitFlowFromFailedSearch;
      _SearchForCozmoFailedScreenInstance.Initialize(_PingStatus);
      _ConnectionFlowBackgroundModalInstance.SetStateFailed(0);
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
    _ConnectionFlowBackgroundModalInstance.SetStateComplete(0);
    ShowConnectingToCozmoScreen();
    // Restart Scan loop sound
    PlayScanLoopAudio(true);
  }

  private void ShowConnectingToCozmoScreen() {
    _ConnectingToCozmoScreenInstance = UIManager.CreateUIElement(_ConnectingToCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<ConnectingToCozmoScreen>();
    _ConnectingToCozmoScreenInstance.ConnectionScreenComplete += HandleConnectingToCozmoScreenDone;
    _ConnectionFlowBackgroundModalInstance.SetStateInProgress(1);

    ConnectToRobot();
  }

  private void HandleConnectingToCozmoScreenDone() {
    GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
    _ConnectionFlowBackgroundModalInstance.SetStateComplete(1);
    ShowSecuringConnectionScreen();
  }

  private void UpdateFirmware() {
    GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
    _UpdateFirmwareScreenInstance = UIManager.CreateUIElement(_UpdateFirmwareScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<UpdateFirmwareScreen>();
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
    ModalPriorityData replaceCozmoPriorityData = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalPrefab.PriorityData);
    UIManager.OpenModal(_ReplaceCozmoOnChargerModalPrefab, replaceCozmoPriorityData, HandleReplaceCozmoOnChargerCreated);
  }

  private void HandleReplaceCozmoOnChargerCreated(BaseModal newSimpleConnectModal) {
    _ReplaceCozmoOnChargerModalInstance = (SimpleConnectModal)newSimpleConnectModal;
    _ReplaceCozmoOnChargerModalInstance.OnConnectButton += ReplaceCozmoOnChargerConnect;
    _ReplaceCozmoOnChargerModalInstance.ModalClosedWithCloseButtonOrOutside += ReturnToTitle;
  }

  private void ReplaceCozmoOnChargerConnect() {
    UIManager.CloseModal(_ReplaceCozmoOnChargerModalInstance);
    ReturnToSearch();
  }

  private void ShowSecuringConnectionScreen() {
    _ConnectionFlowBackgroundModalInstance.SetStateInProgress(2);
    _SecuringConnectionScreenInstance = UIManager.CreateUIElement(_SecuringConnectionScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<SecuringConnectionScreen>();
    _SecuringConnectionScreenInstance.OnScreenComplete += HandleSecuringConnectionScreenDone;
  }

  private void ShowInvalidPinScreen() {
    ModalPriorityData invalidPinModalData = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalPrefab.PriorityData);
    UIManager.OpenModal(_InvalidPinModalPrefab, invalidPinModalData, HandleInvalidPinModalCreated);
  }

  private void HandleInvalidPinModalCreated(BaseModal newInvalidPinModal) {
    _InvalidPinModalInstance = (InvalidPinModal)newInvalidPinModal;
    _InvalidPinModalInstance.OnRetryPin += ShowPinScreen;
    _InvalidPinModalInstance.ModalClosedWithCloseButtonOrOutside += ReturnToTitle;
  }

  private void ShowPinScreen() {
    ModalPriorityData pinSecurityModalData = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalPrefab.PriorityData);
    UIManager.OpenModal(_PinSecurityModalPrefab, pinSecurityModalData, HandlePinSecurityViewCreated);
  }

  private void HandlePinSecurityViewCreated(BaseModal newPinSecurityModal) {
    _PinSecurityModalInstance = (PinSecurityModal)newPinSecurityModal;
    _PinSecurityModalInstance.ModalClosedWithCloseButtonOrOutside += HandlePinSecurityViewClosedByUser;
    _PinSecurityModalInstance.OnPinEntered += HandlePinSecurityEntered;
  }

  private void UpdateAppScreen() {
    ModalPriorityData updateAppModalPrefab = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalPrefab.PriorityData);
    UIManager.OpenModal(_UpdateAppModalPrefab, updateAppModalPrefab, HandleUpdateAppViewCreated);
  }

  private void HandleUpdateAppViewCreated(BaseModal newUpdateAppModal) {
    _UpdateAppModalInstance = (UpdateAppModal)newUpdateAppModal;
    _UpdateAppModalInstance.ModalClosedWithCloseButtonOrOutside += ReturnToTitle;
  }

  private void HandlePinSecurityViewClosedByUser() {
    ReturnToTitle();
  }

  private void HandlePinSecurityEntered(string pin) {
    UIManager.CloseModal(_PinSecurityModalInstance);
    ShowConnectingToCozmoScreen();
  }

  private void HandleSecuringConnectionScreenDone() {
    // Stop scan loop sound connection phase is completed
    PlayScanLoopAudio(false);

    _SecuringConnectionScreenInstance.OnScreenComplete -= HandleSecuringConnectionScreenDone;

    _ConnectionFlowBackgroundModalInstance.SetStateComplete(2);

    // Now that we're connected to robot, make sure everything is unlocked
    if (DebugMenuManager.Instance.DemoMode) {
      UnlockablesManager.Instance.TrySetUnlocked(UnlockId.All, true);
    }

    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow &&
        !DebugMenuManager.Instance.DemoMode) {
      Invoke("TransitionConnectionFlowToPullCubeTabs", ConnectionFlowDelay);
    }
    else {
      GameObject.Destroy(_SecuringConnectionScreenInstance.gameObject);
      CheckForRestoreRobotFlow();
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
    _WakingUpCozmoScreenInstance = UIManager.CreateUIElement(_WakingUpCozmoScreenPrefab, _ConnectionFlowBackgroundModalInstance.transform);

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
    FinishConnectionFlow();
  }

  private void TransitionConnectionFlowToPullCubeTabs() {
    if (_SecuringConnectionScreenInstance != null) {
      GameObject.Destroy(_SecuringConnectionScreenInstance.gameObject);
      _SecuringConnectionScreenInstance = null;
      ModalPriorityData pullCubePriorityData = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalInstance.PriorityData);
      UIManager.CloseModal(_ConnectionFlowBackgroundModalInstance);
      _ConnectionFlowBackgroundModalInstance = null;
      UIManager.OpenModal(_PullCubeTabModalPrefab, pullCubePriorityData, HandlePullCubeTabViewCreated);
    }
    else {
      DAS.Error("TransitionConnectionFlowToPullCubeTabs.CalledMultipleTimes", "should only be called once");
    }
  }

  private void HandlePullCubeTabViewCreated(BaseModal newPullCubeTabModal) {
    _PullCubeTabModalInstance = (PullCubeTabModal)newPullCubeTabModal;
    _PullCubeTabModalInstance.DialogClosed += HandlePullCubeTabsCompleted;
  }

  private void HandlePullCubeTabsCompleted() {
    CheckForRestoreRobotFlow();
  }

  private void FinishConnectionFlow() {
    if (_ConnectionFlowBackgroundModalInstance != null) {
      UIManager.CloseModal(_ConnectionFlowBackgroundModalInstance);
      _ConnectionFlowBackgroundModalInstance = null;
    }

    // explicitly enable charger behavior since it should be off by default in engine.
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("default_disabled", Anki.Cozmo.ReactionTrigger.PlacedOnCharger, true);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("wakeup", Anki.Cozmo.ReactionTrigger.ObjectPositionUpdated, true);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("wakeup", Anki.Cozmo.ReactionTrigger.CubeMoved, true);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("wakeup", Anki.Cozmo.ReactionTrigger.FacePositionUpdated, true);
    }

    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
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
    DAS.Info("ConnectionFlow.HandleRobotConnectResponse", "response: " + response);
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
      if (_ConnectingToCozmoScreenInstance != null) {
        GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
      }
      PlayScanLoopAudio(false);
      ShowPinScreen();
      break;

    case RobotConnectionResult.InvalidPin:
      if (_ConnectingToCozmoScreenInstance != null) {
        GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
      }
      PlayScanLoopAudio(false);
      ShowInvalidPinScreen();
      break;

    case RobotConnectionResult.PinMaxAttemptsReached:
      PlayScanLoopAudio(false);
      ReplaceCozmoOnCharger();
      break;

    case RobotConnectionResult.ConnectionRejected:
      if (_ConnectingToCozmoScreenInstance != null) {
        GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
      }
      PlayScanLoopAudio(false);
      ConnectionRejected();
      break;
    }
  }

  private void ConnectionRejected() {
    _ConnectionFlowBackgroundModalInstance.SetStateFailed(1);
    _ConnectionRejectedScreenInstance = UIManager.CreateUIElement(_ConnectionRejectedScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<ConnectionRejectedScreen>();
    _ConnectionRejectedScreenInstance.OnCancelButton += () => {
      if (_ConnectionRejectedScreenInstance != null) {
        GameObject.Destroy(_ConnectionRejectedScreenInstance.gameObject);
      }
      ReturnToTitle();
    };
    _ConnectionRejectedScreenInstance.OnRetryButton += () => {
      if (_ConnectionRejectedScreenInstance != null) {
        GameObject.Destroy(_ConnectionRejectedScreenInstance.gameObject);
      }
      ReturnToSearch();
    };
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
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("wakeup", Anki.Cozmo.ReactionTrigger.ObjectPositionUpdated, false);
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("wakeup", Anki.Cozmo.ReactionTrigger.CubeMoved, false);
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("wakeup", Anki.Cozmo.ReactionTrigger.FacePositionUpdated, false);
  }

  public void HandleRobotDisconnect() {
    if (_ReplaceCozmoOnChargerModalInstance != null) {
      // don't try to go through the search flow if the replace cozmo on charger view is up.
      return;
    }

    // If we're showing the update app view, the user will need to get a new version, so don't do anything when
    // the robot disconnects
    if (_UpdateAppModalInstance != null) {
      return;
    }

    if (_UpdateFirmwareScreenInstance != null && _UpdateFirmwareScreenInstance.DoneUpdateDelayInProgress) {
      // if we are delaying the firmware screen to wait for robot reboot don't reset the flow during the
      // delay.
      return;
    }

    CancelInvoke("TransitionConnectionFlowToPullCubeTabs");
    Cleanup();
    InitConnectionFlow();
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
