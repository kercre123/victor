using UnityEngine;
using Anki.Assets;
using Cozmo.ConnectionFlow.UI;
using System.Collections.Generic;

namespace Cozmo.ConnectionFlow {
  public class NeedsConnectionManager : MonoBehaviour {

    public static NeedsConnectionManager Instance { get; private set; }

    [SerializeField]
    private GameObjectDataLink _NeedsUnconnectViewPrefabData;
    private NeedsUnconnectedView _NeedsUnconnectViewInstance;

    [SerializeField]
    private HubWorldBase _HubWorldPrefab;
    private HubWorldBase _HubWorldInstance;

    [SerializeField]
    private GameObjectDataLink _ConnectionFlowPrefabData;
    private ConnectionFlowController _ConnectionFlowInstance;

    [SerializeField]
    private GameObjectDataLink _FirstTimeConnectViewPrefabData;
    private FirstTimeConnectView _FirstTimeConnectViewInstance;

    private bool _StartFlowInProgress = false;

    private void OnEnable() {
      if (Instance != null && Instance != this) {
        Destroy(gameObject);
        return;
      }
      else {
        Instance = this;
      }
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
        DAS.Warn("NeedsConnectionManager.StartFlow", "Start flow already in progress");
        return;
      }

      _StartFlowInProgress = true;

      // Before starting, reset some state
      if (DebugMenuManager.Instance.DemoMode) {
        SetupForDemoMode();
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
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
        ShowFirstTimeFlow();
      }
      else {
        ShowNeedsUnconnectedView();
      }
    }

    private void ShowFirstTimeFlow() {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_FirstTimeConnectViewPrefabData.AssetBundle, LoadFirstTimeView);
    }

    private void LoadFirstTimeView(bool assetBundleSuccess) {
      if (assetBundleSuccess) {
        _FirstTimeConnectViewPrefabData.LoadAssetData((GameObject connectViewPrefab) => {
          if (_FirstTimeConnectViewInstance == null && connectViewPrefab != null) {
            UIManager.OpenView(connectViewPrefab.GetComponent<FirstTimeConnectView>(), HandleFirstTimeConnectViewCreated);
          }
        });
      }
      else {
        DAS.Error("NeedsConnectionManager.LoadFirstTimeView", "Failed to load asset bundle " + _FirstTimeConnectViewPrefabData.AssetBundle);
      }
    }
    private void HandleFirstTimeConnectViewCreated(Cozmo.UI.BaseView firstTimeConnectView) {
      _FirstTimeConnectViewInstance = (FirstTimeConnectView)firstTimeConnectView;
      _FirstTimeConnectViewInstance.ConnectionFlowComplete += HandleFirstTimeConnectionFlowComplete;
      _FirstTimeConnectViewInstance.ConnectionFlowQuit += HandleFirstTimeConnectFlowQuit;
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
    private void HandleFirstTimeConnectFlowQuit() {
      // destroy and re-create the connect dialog to restart the flow
      DAS.Info("NeedsConnectionManager.HandleConnectionFlowQuit", "Restarting Connection Dialog Flow");
      if (_FirstTimeConnectViewInstance != null) {
        _FirstTimeConnectViewInstance.ConnectionFlowComplete -= HandleFirstTimeConnectionFlowComplete;
        _FirstTimeConnectViewInstance.ConnectionFlowQuit -= HandleFirstTimeConnectFlowQuit;
        _FirstTimeConnectViewInstance.CloseDialogImmediately();
        _FirstTimeConnectViewInstance = null;
      }
      ShowFirstTimeFlow();
    }

    private void ShowNeedsUnconnectedView() {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_NeedsUnconnectViewPrefabData.AssetBundle, LoadNeedsUnconnectedView);
    }

    private void LoadNeedsUnconnectedView(bool assetBundleSuccess) {
      if (assetBundleSuccess) {
        _NeedsUnconnectViewPrefabData.LoadAssetData((GameObject checkInViewPrefab) => {
          if (_NeedsUnconnectViewInstance == null && checkInViewPrefab != null) {
            UIManager.OpenView(checkInViewPrefab.GetComponent<NeedsUnconnectedView>(), HandleNeedsUnconnectedViewCreated);
          }
        });
      }
      else {
        DAS.Error("NeedsConnectionManager.LoadNeedsUnconnectedView", "Failed to load asset bundle " + _NeedsUnconnectViewPrefabData.AssetBundle);
      }
    }

    private void HandleNeedsUnconnectedViewCreated(Cozmo.UI.BaseView needUnconnectedFlowView) {
      _NeedsUnconnectViewInstance = (NeedsUnconnectedView)needUnconnectedFlowView;
      _NeedsUnconnectViewInstance.OnConnectButtonPressed += HandleConnectButton;
      _NeedsUnconnectViewInstance.OnMockConnectButtonPressed += HandleMockButton;
    }

    private void HandleMockButton() {
      RobotEngineManager.Instance.MockConnect();
      if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
        DataPersistence.DataPersistenceManager.Instance.StartNewSession();
      }
      HandleConnectionFlowComplete();
    }

    private void HandleConnectButton() {
      int daysWithCozmo = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DaysWithCozmo;
      DAS.Event("meta.time_with_cozmo", daysWithCozmo.ToString());
      DAS.Event("meta.games_played", DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.Count.ToString());
      DAS.Event("meta.current_streak", DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CurrentStreak.ToString());

      AssetBundleManager.Instance.LoadAssetBundleAsync(_ConnectionFlowPrefabData.AssetBundle, LoadConnectionFlowPrefab);
    }

    private void LoadConnectionFlowPrefab(bool assetBundleSuccess) {
      if (assetBundleSuccess) {
        _ConnectionFlowPrefabData.LoadAssetData((GameObject connectionFlowPrefab) => {
          if (_ConnectionFlowInstance == null && connectionFlowPrefab != null) {
            _ConnectionFlowInstance = GameObject.Instantiate(connectionFlowPrefab.gameObject).GetComponent<ConnectionFlowController>();
            _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
            _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
            _ConnectionFlowInstance.StartConnectionFlow();
          }
        });
      }
      else {
        DAS.Error("NeedsConnectionManager.LoadConnectionFlowPrefab", "Failed to load asset bundle " + _NeedsUnconnectViewPrefabData.AssetBundle);
      }
    }

    private void HandleConnectionFlowComplete() {
      DestroyConnectionFlowInstance();
      CloseNeedsUnconnectView(true);
      AssetBundleManager.Instance.UnloadAssetBundle(_NeedsUnconnectViewPrefabData.AssetBundle);
      AssetBundleManager.Instance.UnloadAssetBundle(_ConnectionFlowPrefabData.AssetBundle);
      IntroFlowComplete();
    }

    private void IntroFlowComplete() {
      _StartFlowInProgress = false;
      GameObject hubWorldObject = GameObject.Instantiate(_HubWorldPrefab.gameObject);
      hubWorldObject.transform.SetParent(transform, false);
      _HubWorldInstance = hubWorldObject.GetComponent<HubWorldBase>();
      _HubWorldInstance.LoadHubWorld();
    }

    private void HandleConnectionFlowQuit() {
      DestroyConnectionFlowInstance();

      // destroy and re-create the connect dialog to restart the flow
      DAS.Info("NeedsConnectionManager.HandleConnectionFlowQuit", "Restarting NeedsUnconnectedView Flow");
      CloseNeedsUnconnectView(false);

      ShowNeedsUnconnectedView();
    }

    private void SetupForDemoMode() {
      // Set needing onboarding home, but not other phases
      DataPersistence.PlayerProfile profile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
      profile.OnboardingStages[OnboardingManager.OnboardingPhases.InitialSetup] = 0;
      // reset the item inventory
      List<string> itemIDs = Cozmo.ItemDataConfig.GetAllItemIds();
      for (int i = 0; i < itemIDs.Count; ++i) {
        profile.Inventory.SetItemAmount(itemIDs[i], 0);
      }
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Loot);
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Upgrades);
      // Needs to set all difficulties unlocked ( don't just return in UI so that we don't have "new difficulty unlocked popups");
      Challenge.ChallengeData[] challengeList = Challenge.ChallengeDataList.Instance.ChallengeData;
      for (int i = 0; i < challengeList.Length; ++i) {
        profile.GameInstructionalVideoPlayed[challengeList[i].ChallengeID] = true;
        profile.GameDifficulty[challengeList[i].ChallengeID] = challengeList[i].DifficultyOptions.Count;
      }
      // Later on Robot unlocks happen
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
      if (_ConnectionFlowInstance != null) {
        _ConnectionFlowInstance.HandleRobotDisconnect();
      }
      else {
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Connectivity);
        UIManager.EnableTouchEvents();

        if (!_StartFlowInProgress) {
          StartFlow();
        }
      }
    }

    private void DestroyConnectionFlowInstance() {
      if (_ConnectionFlowInstance != null) {
        _ConnectionFlowInstance.ConnectionFlowComplete -= HandleConnectionFlowComplete;
        _ConnectionFlowInstance.ConnectionFlowQuit -= HandleConnectionFlowQuit;
        GameObject.Destroy(_ConnectionFlowInstance.gameObject);
      }
    }

    private void CloseNeedsUnconnectView(bool animate) {
      if (_NeedsUnconnectViewInstance != null) {
        _NeedsUnconnectViewInstance.OnConnectButtonPressed -= HandleConnectButton;
        _NeedsUnconnectViewInstance.OnMockConnectButtonPressed -= HandleMockButton;
        if (animate) {
          _NeedsUnconnectViewInstance.CloseDialog();
        }
        else {
          _NeedsUnconnectViewInstance.CloseDialogImmediately();
        }
        _NeedsUnconnectViewInstance = null;
      }
    }
  }
}