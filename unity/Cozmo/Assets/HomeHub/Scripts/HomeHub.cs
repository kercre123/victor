using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using Cozmo.Util;
using Anki.Assets;

namespace Cozmo.HomeHub {
  public class HomeHub : HubWorldBase {

    private static HomeHub _Instance = null;

    public static HomeHub Instance {
      get {
        if (_Instance == null) {
          DAS.Error("HomeHub.Instance", "NULL HomeHub Instance");
        }
        return _Instance;
      }
    }

    [SerializeField]
    private SerializableAssetBundleNames _MinigameDataPrefabAssetBundle;

    [SerializeField]
    private GameObjectDataLink _StartViewPrefabData;

    private StartView _StartViewInstance;

    [SerializeField]
    private GameObjectDataLink _HomeViewPrefabData;

    private HomeView _HomeViewInstance;

    [SerializeField]
    private GameObjectDataLink _ChallengeDetailsPrefabData;

    private ChallengeDetailsDialog _ChallengeDetailsDialogInstance;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;

    private GameBase _MiniGameInstance;

    public GameBase MiniGameInstance {
      get {
        return _MiniGameInstance;
      }
    }

    private CompletedChallengeData _CurrentChallengePlaying;

    public override bool LoadHubWorld() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshChallengeUnlockInfo);
      _Instance = this;
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      ShowStartView();
      return true;
    }

    public override bool DestroyHubWorld() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshChallengeUnlockInfo);
      CloseMiniGameImmediately();

      // Deregister events
      // Destroy dialog if it exists
      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.CloseViewImmediately();
      }

      if (_StartViewInstance != null) {
        _StartViewInstance.CloseViewImmediately();
      }
      return true;
    }

    private void RefreshChallengeUnlockInfo(object message) {
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      if (_HomeViewInstance != null) {
        _HomeViewInstance.SetChallengeStates(_ChallengeStatesById);
      }
    }

    private void LoadChallengeData(ChallengeDataList sourceChallenges,
                                   out Dictionary<string, ChallengeStatePacket> challengeStateByKey) {
      // Initial load of what's unlocked and completed from data
      challengeStateByKey = new Dictionary<string, ChallengeStatePacket>();

      // For each challenge
      ChallengeStatePacket statePacket;
      foreach (ChallengeData data in sourceChallenges.ChallengeData) {
        // Create a new data packet
        statePacket = new ChallengeStatePacket();
        statePacket.Data = data;

        // Determine the current state of the challenge
        statePacket.ChallengeUnlocked = UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value);
        // Add the challenge to the dictionary
        challengeStateByKey.Add(data.ChallengeID, statePacket);
      }
    }

    private void ShowStartView() {
      RobotEngineManager.Instance.CurrentRobot.SetLiftHeight(0);
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);

      // INGO
      // Right now StartView and ConnectDialog use the same assets so don't bother unloading asset bundle.
      // LoadStartViewAssetBundle(LoadStartView);
      LoadStartView(true);
    }

    private void LoadStartViewAssetBundle(System.Action<bool> loadCallback) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_StartViewPrefabData.AssetBundle, loadCallback);
    }

    private void LoadStartView(bool assetBundleSuccess) {
      _StartViewPrefabData.LoadAssetData((GameObject startViewPrefab) => {
        _StartViewInstance = UIManager.OpenView(startViewPrefab.GetComponent<StartView>());
        _StartViewInstance.OnConnectClicked += HandleConnectClicked;
      });
    }

    private void HandleConnectClicked() {
      _StartViewInstance.OnConnectClicked -= HandleConnectClicked;
      _StartViewInstance.CloseView();
      _StartViewInstance.ViewCloseAnimationFinished += HandleStartViewClosed;
    }

    private void HandleStartViewClosed() {
      _StartViewInstance.ViewCloseAnimationFinished -= HandleStartViewClosed;
      _StartViewInstance = null;
      UnloadStartViewAssetBundle();
      ShowHomeView();
    }

    private void ShowHomeView() {
      LoadHomeViewAssetBundle(LoadHomeView);
    }

    private void UnloadStartViewAssetBundle() {
      AssetBundleManager.Instance.UnloadAssetBundle(_StartViewPrefabData.AssetBundle);
    }

    private void LoadHomeViewAssetBundle(System.Action<bool> loadCallback) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_HomeViewPrefabData.AssetBundle, loadCallback);
    }

    private void LoadHomeView(bool assetBundleSuccess) {
      _HomeViewPrefabData.LoadAssetData((GameObject homeViewPrefab) => {
        _HomeViewInstance = UIManager.OpenView(homeViewPrefab.GetComponent<HomeView>());
        _HomeViewInstance.OnLockedChallengeClicked += HandleLockedChallengeClicked;
        _HomeViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
        _HomeViewInstance.OnCompletedChallengeClicked += HandleCompletedChallengeClicked;
        _HomeViewInstance.OnEndSessionClicked += HandleSessionEndClicked;

        // Show the current state of challenges being locked/unlocked
        _HomeViewInstance.Initialize(_ChallengeStatesById, this);

        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(true);
        DailyGoalManager.Instance.MinigameConfirmed += HandleStartChallengeRequest;

        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Hub);
      });
    }

    private void HandleSessionEndClicked() {
      RobotEngineManager.Instance.CurrentRobot.ResetRobotState(() => {
        CloseTimelineDialog();
        ObservedObject charger = RobotEngineManager.Instance.CurrentRobot.GetCharger();
        if (charger != null) {
          RobotEngineManager.Instance.CurrentRobot.MountCharger(charger, HandleChargerMounted);
        }
        else {
          ShowStartView();
        }
      });
    }

    private void HandleChargerMounted(bool success) {
      ShowStartView();
    }

    private void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // Do nothing for now
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void HandleCompletedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void OpenChallengeDetailsDialog(string challenge, Transform buttonTransform) {
      if (_ChallengeDetailsDialogInstance == null) {
        _ChallengeDetailsPrefabData.LoadAssetData((GameObject challengeDetailsPrefab) => {
          _ChallengeDetailsDialogInstance = UIManager.OpenView(challengeDetailsPrefab.GetComponent<ChallengeDetailsDialog>(),
            newView => {
              newView.Initialize(_ChallengeStatesById[challenge].Data, buttonTransform);
            });

          // React to when we should start the challenge.
          _ChallengeDetailsDialogInstance.ChallengeStarted += HandleStartChallengeClicked;
        });
      }
    }

    private void HandleStartChallengeClicked(string challengeClicked) {
      PlayMinigame(challengeClicked);
    }

    private void HandleStartChallengeRequest(string challengeRequested) {
      PlayMinigame(challengeRequested);
    }

    private void PlayMinigame(string challengeId) {
      // Keep track of the current challenge
      _CurrentChallengePlaying = new CompletedChallengeData() {
        ChallengeId = challengeId,
        StartTime = System.DateTime.UtcNow,
      };

      // Reset the robot behavior
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      }

      // Close dialog
      CloseTimelineDialog();

      LoadMinigameAssetBundle((bool prefabsSuccess) => {
        if (prefabsSuccess) {
          LoadMinigame(_ChallengeStatesById[challengeId].Data);
        }
        else {
          // TODO show error dialog and boot to home
        }
      });
    }

    private void UnloadHomeViewAssetBundle() {
      if (_HomeViewInstance != null) {
        _HomeViewInstance.ViewCloseAnimationFinished -= UnloadHomeViewAssetBundle;
        _HomeViewInstance = null;
      }
      AssetBundleManager.Instance.UnloadAssetBundle(_HomeViewPrefabData.AssetBundle);
    }

    private void LoadMinigameAssetBundle(System.Action<bool> loadCallback) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(
        _MinigameDataPrefabAssetBundle.Value.ToString(), loadCallback);
    }

    private void LoadMinigame(ChallengeData challengeData) {
      challengeData.LoadPrefabData((ChallengePrefabData prefabData) => {
        GameObject newMiniGameObject = Instantiate(prefabData.MinigamePrefab);
        _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
        _MiniGameInstance.InitializeMinigame(challengeData);
        _MiniGameInstance.OnShowEndGameDialog += HandleEndGameDialog;
        _MiniGameInstance.OnMiniGameWin += HandleMiniGameWin;
        _MiniGameInstance.OnMiniGameLose += HandleMiniGameLose;
        _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
        RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.Count);
      });
    }

    private void HandleEndGameDialog() {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(true);
      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
      // TODO : Remove this once we have a more stable, permanent solution in Engine for false cliff detection
      RobotEngineManager.Instance.CurrentRobot.SetEnableCliffSensor(true);
    }

    private void HandleMiniGameWin() {
      HandleMiniGameCompleted(didWin: true);
    }

    private void HandleMiniGameLose() {
      HandleMiniGameCompleted(didWin: false);
    }

    private void HandleMiniGameCompleted(bool didWin) {
      // If we are in a challenge that needs to be completed, complete it
      if (_CurrentChallengePlaying != null) {
        CompleteChallenge(_CurrentChallengePlaying, didWin);
        _CurrentChallengePlaying = null;
      }
      ShowHomeView();
      UnloadMinigameAssetBundle();
    }

    private void HandleMiniGameQuit() {
      // Reset the current challenge
      _CurrentChallengePlaying = null;
      ShowHomeView();
      UnloadMinigameAssetBundle();
    }

    private void UnloadMinigameAssetBundle() {
      AssetBundleManager.Instance.UnloadAssetBundle(_MinigameDataPrefabAssetBundle.Value.ToString());
    }

    private void CompleteChallenge(CompletedChallengeData completedChallenge, bool won) {
      // the last session is not necessarily valid as the 'CurrentSession', as its possible
      // the day rolled over while we were playing the challenge.
      var session = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault();
      if (session == null) {
        DAS.Error(this, "Somehow managed to complete a challenge with no sessions saved!");
      }

      DataPersistenceManager.Instance.Save();
    }

    private void CloseMiniGameImmediately() {
      if (_MiniGameInstance != null) {
        DeregisterMinigameEvents();
        _MiniGameInstance.CloseMinigameImmediately();
      }
    }

    private void DeregisterMinigameEvents() {
      if (_MiniGameInstance != null) {
        _MiniGameInstance.OnMiniGameQuit -= HandleMiniGameQuit;
        _MiniGameInstance.OnMiniGameWin -= HandleMiniGameWin;
        _MiniGameInstance.OnMiniGameLose -= HandleMiniGameLose;
        _MiniGameInstance.OnShowEndGameDialog -= HandleEndGameDialog;
      }
    }

    private void CloseTimelineDialog() {
      if (_ChallengeDetailsDialogInstance != null) {
        _ChallengeDetailsDialogInstance.ChallengeStarted -= HandleStartChallengeClicked;
        _ChallengeDetailsDialogInstance.CloseViewImmediately();
      }
      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.ViewCloseAnimationFinished += UnloadHomeViewAssetBundle;
        _HomeViewInstance.CloseView();
      }
    }

    private void DeregisterDialogEvents() {
      if (_HomeViewInstance != null) {
        _HomeViewInstance.OnLockedChallengeClicked -= HandleLockedChallengeClicked;
        _HomeViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
        _HomeViewInstance.OnCompletedChallengeClicked -= HandleCompletedChallengeClicked;
        _HomeViewInstance.OnEndSessionClicked -= HandleSessionEndClicked;
      }
      DailyGoalManager.Instance.MinigameConfirmed -= HandleStartChallengeRequest;
    }

    #region Testing

    public void TestLoadTimeline() {
      DestroyHubWorld();
      LoadHubWorld();
    }

    public void TestCompleteChallenge(string completedChallengeId) {
      if (_HomeViewInstance != null) {

        CompleteChallenge(new CompletedChallengeData() {
          ChallengeId = completedChallengeId
        }, true);

        // Force refresh of the dialog
        DeregisterDialogEvents();
        _HomeViewInstance.CloseViewImmediately();
        ShowHomeView();
      }
    }

    #endregion
  }

  public class ChallengeStatePacket {
    public ChallengeData Data;
    public bool ChallengeUnlocked;
  }
}
