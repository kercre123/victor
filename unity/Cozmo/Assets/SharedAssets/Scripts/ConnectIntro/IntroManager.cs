using UnityEngine;
using Anki.Assets;
using Cozmo.CheckInFlow.UI;
using System.Collections.Generic;

public class IntroManager : MonoBehaviour {

  private static IntroManager _Instance;

  public static IntroManager Instance {
    get {
      return _Instance;
    }
  }

  [SerializeField]
  private GameObjectDataLink _FirstTimeConnectViewPrefabData;
  private FirstTimeConnectView _FirstTimeConnectViewInstance;

  [SerializeField]
  private AssetBundleNames _PlatformSpecificAssetBundleName;

  [SerializeField]
  private GameObjectDataLink _CheckInViewPrefabData;
  private CheckInFlowView _CheckInViewInstance;

  [SerializeField]
  private HubWorldBase _HubWorldPrefab;
  private HubWorldBase _HubWorldInstance;

  private bool _StartFlowInProgress = false;

  void Awake() {
    if (_Instance != null) {
      DAS.Error("IntroManager.Awake", "There should only be one IntroManager");
      return;
    }
    _Instance = this;
  }

  void Start() {
    Application.targetFrameRate = 30;

    Screen.sleepTimeout = SleepTimeout.NeverSleep;
    Input.gyro.enabled = true;
    Input.compass.enabled = true;
    Input.multiTouchEnabled = false;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);

    StartFlow();

    // If SDK Only, force flag to true, this build flag is also used to hide any options to disable SDK mode
#if SDK_ONLY
    DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled = true;
    DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.SDKActivated = true;
    DataPersistence.DataPersistenceManager.Instance.Save();
#endif

  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);
  }

  private void StartFlow() {
    if (_StartFlowInProgress) {
      DAS.Warn("IntroManager.StartFlow", "Start flow already in progress");
      return;
    }

    _StartFlowInProgress = true;

    // Before starting, reset some state
    if (DebugMenuManager.Instance.DemoMode) {
      // Set needing onboarding home, but not other phases
      DataPersistence.PlayerProfile profile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
      profile.OnboardingStages[OnboardingManager.OnboardingPhases.Home] = 0;
      // reset the item inventory
      List<string> itemIDs = Cozmo.ItemDataConfig.GetAllItemIds();
      for (int i = 0; i < itemIDs.Count; ++i) {
        profile.Inventory.SetItemAmount(itemIDs[i], 0);
      }
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Loot);
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Upgrades);
      // Needs to set all difficulties unlocked ( don't just return in UI so that we don't have "new difficulty unlocked popups");
      ChallengeData[] challengeList = ChallengeDataList.Instance.ChallengeData;
      for (int i = 0; i < challengeList.Length; ++i) {
        profile.GameInstructionalVideoPlayed[challengeList[i].ChallengeID] = true;
        profile.GameDifficulty[challengeList[i].ChallengeID] = challengeList[i].DifficultyOptions.Count;
      }
      // Later on Robot unlocks happen
    }

#if UNITY_ANDROID && !UNITY_EDITOR
    // begin attempting to ping Cozmo now on Android;
    // if we're already connected to his wifi, we want to detect that
    // before we start the Android connection flow
    if (AndroidConnectionFlow.IsAvailable()) {
      AndroidConnectionFlow.StartPingTest();
    }
#endif

    OnboardingManager.Instance.PreloadOnboarding();
    if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home)) {
      ShowFirstTimeFlow();
    }
    else {
      ShowCheckInFlow();
    }
  }

  public void ForceBoot() {
    RobotEngineManager.Instance.StartIdleTimeout(0f, 0f);
    OnRobotDisconnect(new Anki.Cozmo.ExternalInterface.RobotDisconnected());
  }

  private void OnRobotDisconnect(object message) {
    DasTracker.Instance.TrackIntroManagerRobotDisconnect(UIManager.GetTopModalName());
    if (null != _HubWorldInstance) {
      _HubWorldInstance.DestroyHubWorld();
    }

    // if we are in a connection flow then they should handle the disconnects properly.
    if (_FirstTimeConnectViewInstance != null) {
      _FirstTimeConnectViewInstance.HandleRobotDisconnect();
      return;
    }
    else if (_CheckInViewInstance != null && _CheckInViewInstance.gameObject != null) {
      _CheckInViewInstance.HandleRobotDisconnect();
      return;
    }
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Connectivity);
    UIManager.EnableTouchEvents();

    if (!_StartFlowInProgress) {
      StartFlow();
    }
  }

  private void HandleFirstTimeConnectionFlowComplete() {
    AssetBundleManager.Instance.UnloadAssetBundle(_FirstTimeConnectViewPrefabData.AssetBundle);
    if (_FirstTimeConnectViewInstance != null) {
      _FirstTimeConnectViewInstance.ConnectionFlowComplete -= HandleFirstTimeConnectionFlowComplete;
      _FirstTimeConnectViewInstance.ConnectionFlowQuit -= HandleFirstTimeConnectFlowQuit;
      _FirstTimeConnectViewInstance.CloseDialog();
      _FirstTimeConnectViewInstance = null;
    }
    IntroFlowComplete();
  }

  private void HandleCheckinFlowComplete() {
    AssetBundleManager.Instance.UnloadAssetBundle(_CheckInViewPrefabData.AssetBundle);
    if (_CheckInViewInstance != null) {
      _CheckInViewInstance.ConnectionFlowComplete -= HandleCheckinFlowComplete;
      _CheckInViewInstance.CheckInFlowQuit -= HandleCheckInFlowQuit;
      _CheckInViewInstance.CloseDialog();
      _CheckInViewInstance = null;
    }
    IntroFlowComplete();
  }

  private void HandleFirstTimeConnectFlowQuit() {
    // destroy and re-create the connect dialog to restart the flow
    DAS.Info("IntroManager.HandleConnectionFlowQuit", "Restarting Connection Dialog Flow");
    if (_FirstTimeConnectViewInstance != null) {
      _FirstTimeConnectViewInstance.ConnectionFlowComplete -= HandleFirstTimeConnectionFlowComplete;
      _FirstTimeConnectViewInstance.ConnectionFlowQuit -= HandleFirstTimeConnectFlowQuit;
      _FirstTimeConnectViewInstance.CloseDialogImmediately();
      _FirstTimeConnectViewInstance = null;
    }
    ShowFirstTimeConnectDialog();
  }

  private void ShowFirstTimeConnectDialog() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_FirstTimeConnectViewPrefabData.AssetBundle, LoadConnectView);
  }

  private void HandleCheckInFlowQuit() {
    // destroy and re-create the connect dialog to restart the flow
    DAS.Info("IntroManager.HandleCheckInFlowQuit", "Restarting CheckIn Dialog Flow");

    if (_CheckInViewInstance != null) {
      _CheckInViewInstance.ConnectionFlowComplete -= HandleCheckinFlowComplete;
      _CheckInViewInstance.CheckInFlowQuit -= HandleCheckInFlowQuit;
      _CheckInViewInstance.CloseDialogImmediately();
      _CheckInViewInstance = null;
    }

    ShowCheckInFlow();
  }

  private void ShowFirstTimeFlow() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_FirstTimeConnectViewPrefabData.AssetBundle, LoadConnectView);
  }

  private void LoadConnectView(bool assetBundleSuccess) {
    if (assetBundleSuccess) {
      _FirstTimeConnectViewPrefabData.LoadAssetData((GameObject connectViewPrefab) => {
        if (_FirstTimeConnectViewInstance == null && connectViewPrefab != null) {
          UIManager.OpenView(connectViewPrefab.GetComponent<FirstTimeConnectView>(), HandleFirstTimeConnectViewCreated);
        }
      });
    }
    else {
      DAS.Error("IntroManager.LoadConnectView", "Failed to load asset bundle" + _FirstTimeConnectViewPrefabData.AssetBundle);
    }
  }

  private void HandleFirstTimeConnectViewCreated(Cozmo.UI.BaseView firstTimeConnectView) {
    _FirstTimeConnectViewInstance = (FirstTimeConnectView)firstTimeConnectView;
    _FirstTimeConnectViewInstance.ConnectionFlowComplete += HandleFirstTimeConnectionFlowComplete;
    _FirstTimeConnectViewInstance.ConnectionFlowQuit += HandleFirstTimeConnectFlowQuit;
  }

  private void IntroFlowComplete() {
    _StartFlowInProgress = false;
    GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
    hubWorldObject.transform.SetParent(transform, false);
    _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();
    _HubWorldInstance.LoadHubWorld();
  }

  private void ShowCheckInFlow() {
    AssetBundleManager.Instance.LoadAssetBundleAsync(_CheckInViewPrefabData.AssetBundle, LoadCheckInView);
  }

  private void LoadCheckInView(bool assetBundleSuccess) {
    if (assetBundleSuccess) {
      _CheckInViewPrefabData.LoadAssetData((GameObject checkInViewPrefab) => {
        if (_CheckInViewInstance == null && checkInViewPrefab != null) {
          UIManager.OpenView(checkInViewPrefab.GetComponent<CheckInFlowView>(), HandleCheckInViewCreated);
        }
      });
    }
    else {
      DAS.Error("IntroManager.LoadCheckInView", "Failed to load asset bundle " + _CheckInViewPrefabData.AssetBundle);
    }
  }

  private void HandleCheckInViewCreated(Cozmo.UI.BaseView checkInFlowView) {
    _CheckInViewInstance = (CheckInFlowView)checkInFlowView;
    _CheckInViewInstance.ConnectionFlowComplete += HandleCheckinFlowComplete;
    _CheckInViewInstance.CheckInFlowQuit += HandleCheckInFlowQuit;
  }
}
