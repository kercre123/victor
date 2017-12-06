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
  private GameObject _SearchForCozmoScreenInstance;

  [SerializeField]
  private AndroidConnectionFlow _AndroidConnectionFlowPrefab;
  private AndroidConnectionFlow _AndroidConnectionFlowInstance;

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
  private PullCubeTabModal _PullCubeTabModalPrefab;
  private PullCubeTabModal _PullCubeTabModalInstance;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private string _CurrentRobotIP;

  private bool _scanLoopPlaying = false;

  // ConnectionFlow will get destroyed as you cycle back to the main title, and we want 
  // these vars to persist between instances - but not between appruns
  private static String _ConnectionRejectedSSID;

  // For Oreo and above, we're reverting to the old Android connection flow due to the
  // OS being a bit overzealous about wanting to be on an Internet-connected WiFi network (which Cozmo is not)
  private const int _kAndroidOreoSdkVersion = 26;

#if UNITY_EDITOR
  public static bool sManualProgress;
  public static bool ManualSuccess() {
    return Input.GetKeyDown(KeyCode.S);
  }
  public static bool ManualFailure() {
    return Input.GetKeyDown(KeyCode.F);
  }
#endif

  private void Awake() {
    Cozmo.PauseManager.Instance.gameObject.SetActive(false);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(RobotConnectionResponse);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(RobotConnectionResponse);

    Cleanup();

    if (Cozmo.PauseManager.InstanceExists) {
      Cozmo.PauseManager.Instance.gameObject.SetActive(true);
    }
  }

  // only for debug stuff
  private void Update() {
#if UNITY_EDITOR
    if (sManualProgress) {
      if (_SearchForCozmoScreenInstance != null) {
        if (ManualSuccess()) {
          HandleSearchForCozmoScreenDone(true);
        }
        else if (ManualFailure()) {
          HandleSearchForCozmoScreenDone(false);
        }
      }
      else if (_ConnectingToCozmoScreenInstance != null) {
        if (ManualSuccess()) {
          // progress from the connection screen to the next part of the flow
          _ConnectingToCozmoScreenInstance.ConnectionComplete();
        }
        else if (ManualFailure()) {
          HandleRobotConnectResponse(RobotConnectionResult.ConnectionRejected);
        }
      }
    }
#endif
  }

  public static AnimationTrigger GetAnimationForWakeUp() {
    // Previously they quit onboarding in the middle, we need another way to drive off charger
    // other than first onboarding animation.
    if (!OnboardingManager.Instance.AllowFreeplayOnHubEnter()) {
      AnimationTrigger wakeUpTrigger = AnimationTrigger.StartSleeping;
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null && robot.Status(RobotStatusFlag.IS_ON_CHARGER)) {
        wakeUpTrigger = AnimationTrigger.OnboardingWakeUpDriveOffCharger;
      }
      return wakeUpTrigger;
    }
    Cozmo.Needs.NeedsStateManager nsm = Cozmo.Needs.NeedsStateManager.Instance;
    if (nsm != null) {
      if (nsm.GetCurrentDisplayValue(NeedId.Repair).Bracket.Equals(NeedBracketId.Critical)) {
        return AnimationTrigger.ConnectWakeUp_SevereRepair;
      }
      else if (nsm.GetCurrentDisplayValue(NeedId.Energy).Bracket.Equals(NeedBracketId.Critical)) {
        return AnimationTrigger.ConnectWakeUp_SevereEnergy;
      }
    }
    return AnimationTrigger.ConnectWakeUp;
  }

  private void OnApplicationPause(bool bPause) {
    // manually tell engine we are paused/unpaused because we disabled pause manager.
    RobotEngineManager.Instance.SendGameBeingPaused(bPause);
    // flush the message to make sure it is sent
    RobotEngineManager.Instance.FlushChannelMessages();

    // If we're coming back from background, and we're still connected to the same network we were rejected from, DON'T
    // bother pinging Cozmo
    if (!bPause && (!String.IsNullOrEmpty(_ConnectionRejectedSSID) && AndroidConnectionFlow.GetCurrentSSID() != _ConnectionRejectedSSID)) {
      // If we're on the Android flow screen, attempt to start that ping
      if (_AndroidConnectionFlowInstance != null) {
        _AndroidConnectionFlowInstance.SafeStartPingTest();
      }
      else {
        _PingStatus.RestartPing();
      }
    }
  }

  private void DestroyAndroidFlow() {
    if (_AndroidConnectionFlowInstance != null) {
      _AndroidConnectionFlowInstance.Disable();
      GameObject.Destroy(_AndroidConnectionFlowInstance.gameObject);
    }
    _AndroidConnectionFlowInstance = null;
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

    if (_WakingUpCozmoScreenInstance != null) {
      GameObject.Destroy(_WakingUpCozmoScreenInstance.gameObject);
    }

    DestroyAndroidFlow();
  }

  public void StartConnectionFlow() {
    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
      _CurrentRobotIP = RobotEngineManager.kSimRobotIP;
      ConnectionFlowDelay = 0.1f;
    }
    else {
      _CurrentRobotIP = RobotEngineManager.kRobotIP;
    }
    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.UseFastConnectivityFlow) {
      ConnectionFlowDelay = 0.1f;
    }
    InitConnectionFlow();
  }

  private void DestroyBackgroundModal() {
    if (_ConnectionFlowBackgroundModalInstance) {
      UIManager.CloseModal(_ConnectionFlowBackgroundModalInstance);
      _ConnectionFlowBackgroundModalInstance = null;
    }
  }

  private void ReturnToTitle() {
    if (_ConnectionFlowBackgroundModalInstance != null) {
      _ConnectionFlowBackgroundModalInstance.DialogClosed += QuitConnectionFlow;
      DestroyBackgroundModal();
    }
    else {
      QuitConnectionFlow();
    }
    // Always Stop Loop sounds when leaving view
    PlayScanLoopAudio(false);
  }

  private void QuitConnectionFlow() {
    Cozmo.Needs.NeedsStateManager.Instance.SetFullPause(false);

    if (ConnectionFlowQuit != null) {
      ConnectionFlowQuit();
    }
  }

  private void ReturnToSearch() {
    _ConnectionFlowBackgroundModalInstance.ResetAllProgress();
    ShowSearchForCozmo();
  }

  private bool StartAndroidFlowIfApplicable() {
    bool useAndroidFlow = false;
#if UNITY_ANDROID && !UNITY_EDITOR
    // To get around the Unity ping not working on Android 8, we'll just always use the Android flow - which uses
    // a native ping internally
    int deviceSDKVersion = CozmoBinding.GetCurrentActivity().Call<int>("getSDKVersion");
    useAndroidFlow = AndroidConnectionFlow.IsAvailable() && (deviceSDKVersion < _kAndroidOreoSdkVersion);
#endif
#if UNITY_EDITOR
    useAndroidFlow = DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.UseAndroidFlowInMock;
#endif
    if (useAndroidFlow) {
      ShowSearchForCozmoAndroid();
      return true;
    }
    return false;
  }

  private void InitConnectionFlow() {
    Cozmo.Needs.NeedsStateManager.Instance.SetFullPause(true);
    if (StartAndroidFlowIfApplicable()) {
      return;
    }
    // fall-thru from not selecting android flow
    CreateConnectionFlowBackground();
  }

  private void CreateConnectionFlowBackground() {
    CreateConnectionFlowBackgroundWithCallback(ShowSearchForCozmo);
  }

  private void CreateConnectionFlowBackgroundWithCallback(Action onComplete) {
    Action<BaseModal> creationSuccessCallback = (arg) => {
      if (this != null) {
        HandleConnectionFlowBackgroundCreationFinished(arg, onComplete);
      }
    };
    UIManager.OpenModal(_ConnectionFlowBackgroundModalPrefab, new ModalPriorityData(), creationSuccessCallback);
  }

  private void HandleConnectionFlowBackgroundCreationFinished(BaseModal newConnectionFlowBackground, Action launchAction) {
    _ConnectionFlowBackgroundModalInstance = (ConnectionFlowBackgroundModal)newConnectionFlowBackground;
    launchAction();
  }

  private void ShowSearchForCozmoAndroid() {
    DestroyBackgroundModal();

    DAS.Info("ConnectionFlow.ShowAndroid", "Instantiating android flow");
    _AndroidConnectionFlowInstance = GameObject.Instantiate(_AndroidConnectionFlowPrefab.gameObject).GetComponent<AndroidConnectionFlow>();

    // Set up event handlers for things the Android flow might tell us:
    // - if we want to start the flow over again
    _AndroidConnectionFlowInstance.OnRestartFlow += () => {
      // explicitly unregister handlers before instantiating a new instance,
      // since OnDestroy() invocation is delayed until after frame
      DestroyAndroidFlow();
      ShowSearchForCozmoAndroid();
    };
    // - when the android flow starts connecting, transition to the normal search
    //   screen while still monitoring the connection
    _AndroidConnectionFlowInstance.OnStartConnect += () => {
      CreateConnectionFlowBackgroundWithCallback(() => {
        ShowSearchForCozmo();
        _AndroidConnectionFlowInstance.AddConnectingPrefab(_SearchForCozmoScreenInstance);
      });
    };
    // - when the screen finishes, either continue old connection flow accordingly
    //   or go to "search failed" screen
    _AndroidConnectionFlowInstance.OnScreenComplete += (success) => {
      DAS.Info("ConnectionFlow.ShowAndroid", "OnComplete: " + success);
      DestroyAndroidFlow();

      // create background if it doesn't exist yet, then signal we're done
      Action doneAction = () => HandleSearchForCozmoScreenDone(success);
      if (_ConnectionFlowBackgroundModalInstance == null) {
        CreateConnectionFlowBackgroundWithCallback(doneAction);
      }
      else {
        doneAction();
      }
    };
    // - if the old flow is requested, start it
    _AndroidConnectionFlowInstance.OnCancelFlow += () => {
      DAS.Info("ConnectionFlow.ShowAndroid", "CancelAndroid");
      Action cancelFunc = () => {
        DestroyAndroidFlow();
        HandleSearchForCozmoScreenDone(false);
      };

      // We were rejected AND we're still on the same WiFi, we don't want to keep checking for a connection
      if (!String.IsNullOrEmpty(_ConnectionRejectedSSID) && AndroidConnectionFlow.GetCurrentSSID() == _ConnectionRejectedSSID) {
        _PingStatus.PausePing();
      }

      // create background if not created yet
      if (_SearchForCozmoScreenInstance == null) {
        CreateConnectionFlowBackgroundWithCallback(cancelFunc);
      }
      else {
        cancelFunc();
      }
    };

    // Initialize only after we've set up all the handlers
    // A null/empty string means the connection was NOT rejected - therefore, start the ping test
    _AndroidConnectionFlowInstance.Initialize(String.IsNullOrEmpty(_ConnectionRejectedSSID));
  }

  private void ShowSearchForCozmo() {
    _ConnectionFlowBackgroundModalInstance.SetStateInProgress(0);

    if (_SearchForCozmoScreenInstance != null) {
      DAS.Error("ConnectionFlow.SearchForCozmo", "Search screen still exists! Don't create duplicate search screens");
      return;
    }

    var screenInstance = UIManager.CreateUIElement(_SearchForCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<SearchForCozmoScreen>();
    bool androidFlowActive = _AndroidConnectionFlowInstance != null;

    PingStatus pingStatusForSearch = _PingStatus;
#if UNITY_EDITOR
    if (sManualProgress) {
      pingStatusForSearch = null;
    }
#endif
    if (androidFlowActive) {
      pingStatusForSearch = null;
    }
    screenInstance.Initialize(pingStatusForSearch);
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
      // spawn detached so it doesn't inherit this screen's alpha (COZMO-13402)
      _SearchForCozmoFailedScreenInstance = UIManager.CreateUIElement(_SearchForCozmoFailedScreenPrefab.gameObject).GetComponent<SearchForCozmoFailedScreen>();
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
    if (_SearchForCozmoFailedScreenInstance != null) {
      _SearchForCozmoFailedScreenInstance.OnEndpointFound -= HandleEndpointFound;
      _SearchForCozmoFailedScreenInstance.OnQuitFlow -= HandleOnQuitFlowFromFailedSearch;
      GameObject.Destroy(_SearchForCozmoFailedScreenInstance.gameObject);
    }
    if (_ConnectionFlowBackgroundModalInstance != null) {
      _ConnectionFlowBackgroundModalInstance.SetStateComplete(0);
    }
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
    if (_ConnectingToCozmoScreenInstance != null) {
      GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
    }
    _ConnectionFlowBackgroundModalInstance.SetStateComplete(1);
    ShowSecuringConnectionScreen();
  }

  private void UpdateFirmware() {
    if (_ConnectingToCozmoScreenInstance != null) {
      GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);
    }
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
    bool started = StartAndroidFlowIfApplicable();
    if (!started) {
      ReturnToSearch();
    }
  }

  private void ShowSecuringConnectionScreen() {
    _ConnectionFlowBackgroundModalInstance.SetStateInProgress(2);
    _SecuringConnectionScreenInstance = UIManager.CreateUIElement(_SecuringConnectionScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<SecuringConnectionScreen>();
    _SecuringConnectionScreenInstance.OnScreenComplete += HandleSecuringConnectionScreenDone;
  }

  private void UpdateAppScreen() {
    ModalPriorityData updateAppModalPrefab = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalInstance.PriorityData);
    UIManager.OpenModal(_UpdateAppModalPrefab, updateAppModalPrefab, HandleUpdateAppViewCreated);
  }

  private void HandleUpdateAppViewCreated(BaseModal newUpdateAppModal) {
    _UpdateAppModalInstance = (UpdateAppModal)newUpdateAppModal;
    _UpdateAppModalInstance.ModalClosedWithCloseButtonOrOutside += ReturnToTitle;
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
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      // we've never connected before so still have setup
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
        // we are done with first time user flow..
        DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow = false;
        DataPersistence.DataPersistenceManager.Instance.Save();

        // Need to create the struct that will be written to the NVEntryTag, set values in it, and then pack it into a
        // memoryStream which will modify the byte array it was constructed with. That byte array is then what is
        // passed to the NVStorageWrite message.
        Anki.Cozmo.OnboardingData data = new Anki.Cozmo.OnboardingData();
        data.hasCompletedOnboarding = true;
        byte[] byteArr = new byte[data.Size];
        System.IO.MemoryStream ms = new System.IO.MemoryStream(byteArr);
        data.Pack(ms);

        robot.NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_OnboardingData, byteArr);
        robot.SendAnimationTrigger(AnimationTrigger.StartSleeping);


        FinishConnectionFlow();
      }
      // connected before but quit while the "special moments" were still happening so play again rather than normal wakeup
      else if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.InitialSetup)) {
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.StartSleeping);
        FinishConnectionFlow();
      }
      // normal flow
      else {
        WakeupSequence();
      }
    }
    else {
      // Robot has disconnected cleanup in HandleRobotDisconnect
      DAS.Info("ConnectionFlowController.CheckForRestoreRobotFlow", "Robot has disconnected");
    }
  }

  private void WakeupSequence() {

    //Disable some reactionary behaviors during wakeup
    RobotEngineManager.Instance.CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kWakeupId, ReactionaryBehaviorEnableGroups.kWakeupTriggers);

    _WakingUpCozmoScreenInstance = UIManager.CreateUIElement(_WakingUpCozmoScreenPrefab, _ConnectionFlowBackgroundModalInstance.transform);
    if (DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled) {
      // just a quick animation to kick the eyes on in SDK mode, but not hold for seconds
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(AnimationTrigger.NeutralFace, HandleWakeAnimationComplete);
    }
    else if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
      HandleWakeAnimationComplete(true);
    }
    else {
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(GetAnimationForWakeUp(), HandleWakeAnimationComplete);
      RobotEngineManager.Instance.Message.NotifyCozmoWakeup = Singleton<Anki.Cozmo.ExternalInterface.NotifyCozmoWakeup>.Instance;
      RobotEngineManager.Instance.SendMessage();
    }

  }

  private void HandleWakeAnimationComplete(bool success) {
    if (_WakingUpCozmoScreenInstance != null) {
      GameObject.Destroy(_WakingUpCozmoScreenInstance);
    }
    DAS.Debug("ConnectionFlow.HandleWakeAnimationComplete", "wake up animation: " + success);
    FinishConnectionFlow();
  }

  private void TransitionConnectionFlowToPullCubeTabs() {
    if (_SecuringConnectionScreenInstance != null) {
      GameObject.Destroy(_SecuringConnectionScreenInstance.gameObject);
      _SecuringConnectionScreenInstance = null;
      ModalPriorityData pullCubePriorityData = ModalPriorityData.CreateSlightlyHigherData(_ConnectionFlowBackgroundModalInstance.PriorityData);
      DestroyBackgroundModal();
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
    DestroyBackgroundModal();

    Cozmo.Needs.NeedsStateManager.Instance.SetFullPause(false);

    // explicitly enable wake up behaviors that are off in engine by default.
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kWakeupId);
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
    DAS.Warn("ConnectionFlow.Disconnected", "Robot Disconnected");
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
      // No longer used
      break;

    case RobotConnectionResult.InvalidPin:
      // No longer used
      break;

    case RobotConnectionResult.PinMaxAttemptsReached:
      // No longer used
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
    _ConnectionRejectedSSID = AndroidConnectionFlow.GetCurrentSSID();

    _ConnectionFlowBackgroundModalInstance.SetStateFailed(1);
    _ConnectionRejectedScreenInstance = UIManager.CreateUIElement(_ConnectionRejectedScreenPrefab.gameObject, _ConnectionFlowBackgroundModalInstance.transform).GetComponent<ConnectionRejectedScreen>();
    _ConnectionRejectedScreenInstance.OnCancelButton += () => {
      if (_ConnectionRejectedScreenInstance != null) {
        GameObject.Destroy(_ConnectionRejectedScreenInstance.gameObject);
      }
#if !UNITY_EDITOR && UNITY_ANDROID
      AndroidConnectionFlow.CallJava("disconnect");
#endif
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
    DataPersistence.DataPersistenceManager.Instance.SetHasConnectedWithCozmo();
    // Successfully connected, reset state
    _ConnectionRejectedSSID = null;

    // When we are in the first time user flow, we enable the block pool when we get to the Pull Cube Tab screen
    if (!DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
      // Enable the automatic block pool
#if ANKI_DEV_CHEATS
      RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.EnableAutoBlockPoolOnStart);
#else
      RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(true);
#endif
    }
  }

  public bool ShouldIgnoreRobotDisconnect() {
    // If we're showing the update app view, the user will need to get a new version, so don't do anything when
    // the robot disconnects
    if (_UpdateAppModalInstance != null) {
      return true;
    }

    if (_UpdateFirmwareScreenInstance != null && _UpdateFirmwareScreenInstance.DoneUpdateDelayInProgress) {
      // if we are delaying the firmware screen to wait for robot reboot don't reset the flow during the
      // delay.
      return true;
    }

    return false;
  }

  public void HandleRobotDisconnect() {
    if (ShouldIgnoreRobotDisconnect()) {
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
    Anki.AudioMetaData.GameEvent.Ui evt = _scanLoopPlaying ? Anki.AudioMetaData.GameEvent.Ui.Cozmo_Connect_Scan_Loop : Anki.AudioMetaData.GameEvent.Ui.Cozmo_Connect_Scan_Loop_Stop;
    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(evt);
  }
}
