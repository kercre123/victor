using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using Cozmo.Util;
using Anki.Assets;
using Anki.Cozmo;
using Cozmo.UI;

namespace Cozmo.HomeHub {
  public class HomeHub : HubWorldBase {

    private static HomeHub _Instance = null;

    public static HomeHub Instance { get { return _Instance; } }

    [SerializeField]
    private SerializableAssetBundleNames _MinigameDataPrefabAssetBundle;

    [SerializeField]
    private GameObjectDataLink _HomeViewPrefabData;

    private HomeView _HomeViewInstance;

    public HomeView HomeViewInstance {
      get {
        return _HomeViewInstance;
      }
    }

    [SerializeField]
    private GameObjectDataLink _ChallengeDetailsPrefabData;

    private ChallengeDetailsDialog _ChallengeDetailsDialogInstance;

    public bool IsChallengeDetailsActive {
      get {
        return _ChallengeDetailsDialogInstance != null;
      }
    }

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

    private AnimationTrigger _MinigameGetOutAnimTrigger = AnimationTrigger.Count;

    public override void LoadHubWorld() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshChallengeUnlockInfo);
      _Instance = this;
      LoadChallengeData(_ChallengeDataList, out _ChallengeStatesById);
      StartLoadHomeView();
    }

    public override void DestroyHubWorld() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshChallengeUnlockInfo);
      CloseMiniGameImmediately();
      if (_ChallengeDetailsDialogInstance != null) {
        _ChallengeDetailsDialogInstance.CloseViewImmediately();
      }
      // Deregister events
      // Destroy dialog if it exists
      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.CloseViewImmediately();
      }
      // Kill yourself HomeHub
      GameObject.Destroy(this.gameObject);
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

    private void StartLoadHomeView() {
      LoadHomeViewAssetBundle(LoadHomeView);
    }

    private void LoadHomeViewAssetBundle(System.Action<bool> loadCallback) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_HomeViewPrefabData.AssetBundle, loadCallback);
    }

    private void LoadHomeView(bool assetBundleSuccess) {
      if (assetBundleSuccess) {
        _HomeViewPrefabData.LoadAssetData((GameObject homeViewPrefab) => {
          if (homeViewPrefab != null) {
            StartCoroutine(ShowHomeViewAfterOtherViewClosed(homeViewPrefab));
          }
          else {
            DAS.Error("HomeHub.LoadHomeView", "HomeViewPrefab is null");
          }
        });
      }
      else {
        DAS.Error("HomeHub.LoadHomeView", "Failed to load asset bundle " + _HomeViewPrefabData.AssetBundle);
      }
    }

    private IEnumerator ShowHomeViewAfterOtherViewClosed(GameObject homeViewPrefab) {

      // wait until minigame instance is destroyed and also wait until the unlocks are properly loaded from
      // robot.
      while (_MiniGameInstance != null || !UnlockablesManager.Instance.UnlocksLoaded) {

        yield return 0;
      }

      _HomeViewInstance = UIManager.OpenView(homeViewPrefab.GetComponent<HomeView>());
      _HomeViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
      _HomeViewInstance.MinigameConfirmed += HandleStartChallengeRequest;

      // Show the current state of challenges being locked/unlocked
      _HomeViewInstance.Initialize(_ChallengeStatesById, this);

      ResetRobotToFreeplaySettings();

      var robot = RobotEngineManager.Instance.CurrentRobot;

      OnboardingManager.Instance.InitHomeHubOnboarding(_HomeViewInstance);

      if (robot != null && !OnboardingManager.Instance.IsAnyOnboardingActive()) {
        // Display Cozmo's default face
        robot.DisplayProceduralFace(
          0,
          Vector2.zero,
          Vector2.one,
          ProceduralEyeParameters.MakeDefaultLeftEye(),
          ProceduralEyeParameters.MakeDefaultRightEye());

        robot.ResetRobotState(() => {
          robot.SendAnimationTrigger(_MinigameGetOutAnimTrigger, (bool success) => {
            _MinigameGetOutAnimTrigger = AnimationTrigger.Count;
            StartFreeplay(robot);
          });
        });
      }

    }

    public void StartFreeplay(IRobot robot) {

      if (!DataPersistenceManager.Instance.Data.DebugPrefs.NoFreeplayOnStart &&
          !OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home)) {
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
        robot.SetEnableFreeplayLightStates(true);
        robot.SetEnableFreeplayBehaviorChooser(true);
      }
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void OpenChallengeDetailsDialog(string challenge, Transform buttonTransform) {
      if (_ChallengeDetailsDialogInstance == null) {
        bool available = UnlockablesManager.Instance.IsUnlockableAvailable(_ChallengeStatesById[challenge].Data.UnlockId.Value);
        UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(_ChallengeStatesById[challenge].Data.UnlockId.Value);
        if (!available) {

          UnlockableInfo preReqInfo = null;
          for (int i = 0; i < unlockInfo.Prerequisites.Length; i++) {
            // if available but we haven't unlocked it yet, then it is the upgrade that is blocking us
            if (!UnlockablesManager.Instance.IsUnlocked(unlockInfo.Prerequisites[i].Value)) {
              preReqInfo = UnlockablesManager.Instance.GetUnlockableInfo(unlockInfo.Prerequisites[i].Value);
            }
          }

          // Create alert view with Icon
          AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: true);
          alertView.SetPrimaryButton(LocalizationKeys.kButtonClose, null);
          alertView.TitleLocKey = unlockInfo.TitleKey;

          string preReqTypeKey = "unlockable.Unlock";

          switch (preReqInfo.UnlockableType) {
          case UnlockableType.Action:
            preReqTypeKey = LocalizationKeys.kUnlockableUpgrade;
            break;
          case UnlockableType.Game:
            preReqTypeKey = LocalizationKeys.kUnlockableApp;
            break;
          default:
            preReqTypeKey = LocalizationKeys.kUnlockableUnlock;
            break;
          }

          alertView.DescriptionLocKey = LocalizationKeys.kUnlockablePreReqNeededDescription;
          alertView.SetMessageArgs(new object[] { Localization.Get(preReqInfo.TitleKey), Localization.Get(preReqTypeKey) });
        }
        else {
          // Throttle Multitouch bug potential
          _HomeViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;

          _ChallengeDetailsPrefabData.LoadAssetData((GameObject challengeDetailsPrefab) => {
            _ChallengeDetailsDialogInstance = UIManager.OpenView(challengeDetailsPrefab.GetComponent<ChallengeDetailsDialog>(),
              newView => {
                newView.Initialize(_ChallengeStatesById[challenge].Data);
              });

            _HomeViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
            // React to when we should start the challenge.
            _ChallengeDetailsDialogInstance.ChallengeStarted += HandleStartChallengeClicked;
          });
        }

      }
    }

    private void HandleStartChallengeClicked(string challengeClicked) {
      PlayMinigame(challengeClicked);
    }

    private void HandleStartChallengeRequest(string challengeRequested) {
      PlayMinigame(challengeRequested, true);
    }

    private void PlayMinigame(string challengeId, bool wasRequest = false) {
      // Keep track of the current challenge
      _CurrentChallengePlaying = new CompletedChallengeData() {
        ChallengeId = challengeId,
        StartTime = System.DateTime.UtcNow,
      };

      // Reset the robot behavior
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
        // If accepted a request, because we've turned off freeplay behavior
        // we need to send Cozmo their animation from unity.
        if (wasRequest) {
          RequestGameListConfig rcList = DailyGoalManager.Instance.GetRequestMinigameConfig();
          RequestGameConfig rc = null;
          for (int i = 0; i < rcList.RequestList.Length; i++) {
            if (rcList.RequestList[i].ChallengeID == challengeId) {
              rc = rcList.RequestList[i];
              break;
            }
          }
          if (rc != null) {
            RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(rc.RequestAcceptedAnimationTrigger.Value);
          }
        }
      }

      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeStarted, challengeId, wasRequest));

      // Close dialog
      CloseHomeView();
    }

    private void HandleHomeViewCloseAnimationFinished() {
      if (_HomeViewInstance != null) {
        _HomeViewInstance.ViewCloseAnimationFinished -= HandleHomeViewCloseAnimationFinished;
        _HomeViewInstance = null;
      }
      AssetBundleManager.Instance.UnloadAssetBundle(_HomeViewPrefabData.AssetBundle);

      LoadMinigameAssetBundle((bool prefabsSuccess) => {
        if (prefabsSuccess) {
          LoadMinigame(_ChallengeStatesById[_CurrentChallengePlaying.ChallengeId].Data);
        }
        else {
          // TODO show error dialog and boot to home
          DAS.Error("HomeHub.HandleHomeViewCloseAnimationFinished", "Failed to load asset bundle " + _MinigameDataPrefabAssetBundle.Value.ToString());
        }
      });
    }

    private void LoadMinigameAssetBundle(System.Action<bool> loadCallback) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(
        _MinigameDataPrefabAssetBundle.Value.ToString(), loadCallback);
    }

    private void LoadMinigame(ChallengeData challengeData) {
      challengeData.LoadPrefabData((ChallengePrefabData prefabData) => {
        // Set the GetOut animation to play when this minigame is destroyed
        _MinigameGetOutAnimTrigger = challengeData.GetOutAnimTrigger.Value;
        StartCoroutine(ShowMinigameAfterHomeViewCloses(challengeData, prefabData));
      });
    }

    private IEnumerator ShowMinigameAfterHomeViewCloses(ChallengeData challengeData, ChallengePrefabData prefabData) {
      while (_HomeViewInstance != null || _ChallengeDetailsDialogInstance != null || _MiniGameInstance != null) {
        yield return 0;
      }

      GameObject newMiniGameObject = Instantiate(prefabData.MinigamePrefab);
      _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();

      // OnSharedMinigameViewInitialized is called as part of the InitializeMinigame flow; 
      // On device this involves loading assets from data but in editor it may be instantaneous
      // so we need to listen to the event first and then initialize
      _MiniGameInstance.OnSharedMinigameViewInitialized += HandleSharedMinigameViewInitialized;
      _MiniGameInstance.InitializeMinigame(challengeData);

      _MiniGameInstance.OnShowEndGameDialog += HandleEndGameDialog;
      _MiniGameInstance.OnMiniGameWin += HandleMiniGameWin;
      _MiniGameInstance.OnMiniGameLose += HandleMiniGameLose;
      _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;

      RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.Count);
    }

    private void HandleSharedMinigameViewInitialized(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _MiniGameInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
      newView.ViewCloseAnimationFinished += HandleMinigameFinishedClosing;
    }

    private void HandleEndGameDialog() {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(true);
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayLightStates(true);
      ResetRobotToFreeplaySettings();
    }

    private void ResetRobotToFreeplaySettings() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (null != robot) {
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
        // TODO : Remove this once we have a more stable, permanent solution in Engine for false cliff detection
        robot.SetEnableCliffSensor(true);
      }
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
        HandleMiniGameQuit();
      }
    }

    private void HandleMiniGameQuit() {
      // Reset the current challenge and re-register the HandleStartChallengeRequest
      _CurrentChallengePlaying = null;
    }

    private void HandleMinigameFinishedClosing() {
      _MiniGameInstance.SharedMinigameView.ViewCloseAnimationFinished -= HandleMinigameFinishedClosing;
      UnloadMinigameAssetBundle();
      StartLoadHomeView();
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

    public void CloseMiniGameImmediately() {
      if (_MiniGameInstance != null) {
        DeregisterMinigameEvents();
        _MiniGameInstance.CloseMinigameImmediately();
        _MiniGameInstance = null;
        UnloadMinigameAssetBundle();
        StartLoadHomeView();
      }
    }

    private void DeregisterMinigameEvents() {
      if (_MiniGameInstance != null) {
        _MiniGameInstance.OnMiniGameQuit -= HandleMiniGameQuit;
        _MiniGameInstance.OnMiniGameWin -= HandleMiniGameWin;
        _MiniGameInstance.OnMiniGameLose -= HandleMiniGameLose;
        _MiniGameInstance.OnShowEndGameDialog -= HandleEndGameDialog;
        _MiniGameInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
        if (MiniGameInstance.SharedMinigameView != null) {
          _MiniGameInstance.SharedMinigameView.ViewCloseAnimationFinished -= HandleMinigameFinishedClosing;
        }
      }
    }

    private void CloseHomeView() {
      if (_ChallengeDetailsDialogInstance != null) {
        _ChallengeDetailsDialogInstance.ChallengeStarted -= HandleStartChallengeClicked;
        _ChallengeDetailsDialogInstance.ViewCloseAnimationFinished += HandleCloseDetailsDialogAnimationFinished;
        _ChallengeDetailsDialogInstance.CloseView();
      }

      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.ViewCloseAnimationFinished += HandleHomeViewCloseAnimationFinished;
        _HomeViewInstance.CloseView();
      }
    }

    private void HandleCloseDetailsDialogAnimationFinished() {
      _ChallengeDetailsDialogInstance.ViewCloseAnimationFinished -= HandleCloseDetailsDialogAnimationFinished;
      _ChallengeDetailsDialogInstance = null;
    }

    private void DeregisterDialogEvents() {
      if (_HomeViewInstance != null) {
        _HomeViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
        _HomeViewInstance.MinigameConfirmed -= HandleStartChallengeRequest;
      }
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
        StartLoadHomeView();
      }
    }

    #endregion
  }

  public class ChallengeStatePacket {
    public ChallengeData Data;
    public bool ChallengeUnlocked;
  }
}
