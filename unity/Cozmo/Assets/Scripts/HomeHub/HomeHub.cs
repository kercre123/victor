using Anki.Assets;
using Anki.Cozmo;
using Cozmo.Challenge;
using Cozmo.UI;
using Cozmo.Util;
using Cozmo.RequestGame;
using DataPersistence;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.HomeHub {
  public class HomeHub : HubWorldBase {

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
    private GameObjectDataLink _ChallengeDetailsModalPrefabData;

    private ModalPriorityData _ChallengeDetailsPriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                                                    LowPriorityModalAction.CancelSelf,
                                                                                    HighPriorityModalAction.Stack);
    private BaseModal _ChallengeDetailsModalInstance;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStatesById;

    private GameBase _MiniGameInstance;

    private CompletedChallengeData _CurrentChallengePlaying;

    private AnimationTrigger _MinigameGetOutAnimTrigger = AnimationTrigger.Count;

    // Total ConnectedTime For GameEvents
    private const float _kConnectedTimeIntervalCheck = 60.0f;
    private float _ConnectedTimeIntervalLastTimestamp = -1;
    private float _ConnectedTimeStartedTimestamp = -1;

    public override void LoadHubWorld() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshChallengeUnlockInfo);
      _Instance = this;
      LoadChallengeData(ChallengeDataList.Instance, out _ChallengeStatesById);
      StartLoadHomeView();

      _ConnectedTimeStartedTimestamp = Time.time;
      _ConnectedTimeIntervalLastTimestamp = _ConnectedTimeStartedTimestamp;
    }

    public override void DestroyHubWorld() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(RefreshChallengeUnlockInfo);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FeedingSFXStageUpdate>(HandleFeedingSFXStageUpdate);
      CloseChallengeImmediately();
      if (_ChallengeDetailsModalInstance != null) {
        _ChallengeDetailsModalInstance.CloseDialogImmediately();
      }
      // Deregister events
      // Destroy dialog if it exists
      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();

        // Since HomeView is a BaseView, it will be closed the next time a view opens. 
        // This is only called during robot disconnect so it should be closed right away by
        // the opening of the CheckInRewardsFlow
        // _HomeViewInstance.CloseDialogImmediately();
      }
      // Kill yourself HomeHub
      GameObject.Destroy(this.gameObject);
    }

    public override GameBase GetChallengeInstance() {
      return _MiniGameInstance;
    }

    private void RefreshChallengeUnlockInfo(object message) {
      LoadChallengeData(ChallengeDataList.Instance, out _ChallengeStatesById);
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
          // this can be null if, by the time this callback is called, HomeHub has been destroyed.
          // This seems to be happening in some disconnection cases and a NRE is thrown
          if (this != null) {
            if (homeViewPrefab != null) {
              StartCoroutine(ShowHomeViewAfterOtherViewClosed(homeViewPrefab));
            }
            else {
              DAS.Error("HomeHub.LoadHomeView", "HomeViewPrefab is null");
            }
          }
        });
      }
      else {
        DAS.Error("HomeHub.LoadHomeView", "Failed to load asset bundle " + _HomeViewPrefabData.AssetBundle);
      }
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FeedingSFXStageUpdate>(HandleFeedingSFXStageUpdate);
    }

    private void HandleFeedingSFXStageUpdate(Anki.Cozmo.ExternalInterface.FeedingSFXStageUpdate message) {
      uint stageNum = message.stage;
      switch (stageNum) {
      case 0: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Loop_Play);
          break;
        }
      case 1: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Up);
          break;
        }
      case 2: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Down);
          break;
        }
      case 3: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Success);
          break;
        }
      case 4: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Loop_Stop);
          break;
        }
      }
    }

    private IEnumerator ShowHomeViewAfterOtherViewClosed(GameObject homeViewPrefab) {

      // wait until minigame instance is destroyed and also wait until the unlocks are properly loaded from
      // robot.
      while (_MiniGameInstance != null || !UnlockablesManager.Instance.UnlocksLoaded) {

        yield return 0;
      }

      UIManager.OpenView(homeViewPrefab.GetComponent<HomeView>(), (newHomeView) => {
        _HomeViewInstance = (HomeView)newHomeView;
        _HomeViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
        RequestGameManager.Instance.OnRequestGameConfirmed += HandleStartChallengeRequest;

        // Show the current state of challenges being locked/unlocked
        _HomeViewInstance.Initialize(_ChallengeStatesById, this);

        ResetRobotToFreeplaySettings();

        var robot = RobotEngineManager.Instance.CurrentRobot;

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
      });
    }

    public override void StartFreeplay(IRobot robot) {

      if (!DataPersistenceManager.Instance.Data.DebugPrefs.NoFreeplayOnStart &&
          !OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Home) &&
          robot != null) {
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Freeplay);
        robot.SetEnableFreeplayLightStates(true);
        robot.SetEnableFreeplayActivity(true);
      }
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      OpenChallengeDetailsDialog(challengeClicked, buttonTransform);
    }

    private void OpenChallengeDetailsDialog(string challenge, Transform buttonTransform) {
      if (_ChallengeDetailsModalInstance == null) {
        bool available = UnlockablesManager.Instance.IsUnlockableAvailable(_ChallengeStatesById[challenge].Data.UnlockId.Value);
        UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(_ChallengeStatesById[challenge].Data.UnlockId.Value);
        if (unlockInfo.ComingSoon) {
          OpenComingSoonAlert();
        }
        else if (!UnlockablesManager.Instance.IsOSSupported(unlockInfo)) {
          OpenOSUnsupportedDialog(unlockInfo, challenge);
        }
        else if (!available) {
          OpenNotAvailableDialog(unlockInfo, challenge);
        }
        else {
          OpenAvailableDialog(challenge);
        }
      }
    }

    private void OpenComingSoonAlert() {
      var comingSoonAlertData = new AlertModalData("activity_coming_soon_alert",
                                                   LocalizationKeys.kUnlockableComingSoonTitle,
                                                   LocalizationKeys.kUnlockableComingSoonDescription,
                                                   new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      UIManager.OpenAlert(comingSoonAlertData, _ChallengeDetailsPriorityData, HandleChallengeAlertViewCreated,
                          overrideCloseOnTouchOutside: true);
    }

    private void OpenOSUnsupportedDialog(UnlockableInfo unlockInfo, string challenge) {
      var descLocArgs = new object[] { unlockInfo.AndroidReleaseVersion };

      var notAvailableAlertData = new AlertModalData("activity_locked_alert",
                           unlockInfo.TitleKey,
                           LocalizationKeys.kUnlockableOSNotSupportedDescription,
                           new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                           icon: _ChallengeStatesById[challenge].Data.ChallengeIcon,
                           descLocArgs: descLocArgs);

      UIManager.OpenAlert(notAvailableAlertData, _ChallengeDetailsPriorityData, HandleChallengeAlertViewCreated,
                                overrideCloseOnTouchOutside: true);
    }

    private void OpenNotAvailableDialog(UnlockableInfo unlockInfo, string challenge) {
      UnlockableInfo preReqInfo = null;
      for (int i = 0; i < unlockInfo.Prerequisites.Length; i++) {
        // if available but we haven't unlocked it yet, then it is the upgrade that is blocking us
        if (!UnlockablesManager.Instance.IsUnlocked(unlockInfo.Prerequisites[i].Value)) {
          preReqInfo = UnlockablesManager.Instance.GetUnlockableInfo(unlockInfo.Prerequisites[i].Value);
        }
      }
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

      var descLocArgs = new object[] { Localization.Get(preReqInfo.TitleKey), Localization.Get(preReqTypeKey) };

      var notAvailableAlertData = new AlertModalData("activity_locked_alert",
                                                     unlockInfo.TitleKey,
                                                     LocalizationKeys.kUnlockablePreReqNeededDescription,
                                                     new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                                     icon: _ChallengeStatesById[challenge].Data.ChallengeIcon,
                                                     descLocArgs: descLocArgs);

      UIManager.OpenAlert(notAvailableAlertData, _ChallengeDetailsPriorityData, HandleChallengeAlertViewCreated,
                          overrideCloseOnTouchOutside: true);
    }

    private void HandleChallengeAlertViewCreated(AlertModal alertModal) {
      if (_ChallengeDetailsModalInstance != null) {
        _ChallengeDetailsModalInstance.CloseDialogImmediately();
      }
      _ChallengeDetailsModalInstance = alertModal;
    }

    private void OpenAvailableDialog(string challenge) {
      // Throttle Multitouch bug potential
      _HomeViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;

      System.Action<BaseModal> detailsModalCreated = (newModal) => {
        // React to when we should start the challenge.
        _ChallengeDetailsModalInstance = newModal;
        ChallengeDetailsModal challengeModal = (ChallengeDetailsModal)newModal;
        challengeModal.InitializeChallengeData(_ChallengeStatesById[challenge].Data, _HomeViewInstance);
        challengeModal.ChallengeStarted += HandleStartChallengeClicked;
      };

      System.Action<GameObject> detailsModalLoaded = (GameObject challengeDetailsPrefab) => {
        UIManager.OpenModal(challengeDetailsPrefab.GetComponent<ChallengeDetailsModal>(), _ChallengeDetailsPriorityData, detailsModalCreated);
        _HomeViewInstance.OnUnlockedChallengeClicked += HandleUnlockedChallengeClicked;
      };

      _ChallengeDetailsModalPrefabData.LoadAssetData(detailsModalLoaded);
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
        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayActivity(false);
        // If accepted a request, because we've turned off freeplay behavior
        // we need to send Cozmo their animation from unity.
        if (wasRequest) {
          RequestGameListConfig rcList = RequestGameListConfig.Instance;
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
        _HomeViewInstance.DialogCloseAnimationFinished -= HandleHomeViewCloseAnimationFinished;
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

        // This can be null if the HomeHub is destroyed before this callback is executed
        if (this != null) {
          StartCoroutine(ShowMinigameAfterHomeViewCloses(challengeData, prefabData));
        }
      });
    }

    private IEnumerator ShowMinigameAfterHomeViewCloses(ChallengeData challengeData, ChallengePrefabData prefabData) {
      while (_HomeViewInstance != null || _ChallengeDetailsModalInstance != null) {
        yield return 0;
      }

      GameObject newMiniGameObject = Instantiate(prefabData.ChallengePrefab);
      _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();

      // OnSharedMinigameViewInitialized is called as part of the InitializeMinigame flow; 
      // On device this involves loading assets from data but in editor it may be instantaneous
      // so we need to listen to the event first and then initialize
      _MiniGameInstance.OnSharedMinigameViewInitialized += HandleSharedMinigameViewInitialized;
      _MiniGameInstance.InitializeChallenge(challengeData);

      _MiniGameInstance.OnShowEndGameDialog += HandleEndGameDialog;
      _MiniGameInstance.OnChallengeWin += HandleMiniGameWin;
      _MiniGameInstance.OnChallengeLose += HandleMiniGameLose;
      _MiniGameInstance.OnChallengeQuit += HandleMiniGameQuit;

      RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.Count);
    }

    private void HandleSharedMinigameViewInitialized(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _MiniGameInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
      newView.DialogCloseAnimationFinished += HandleMinigameFinishedClosing;
    }

    private void HandleEndGameDialog() {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayActivity(true);
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayLightStates(true);
      ResetRobotToFreeplaySettings();
    }

    private void ResetRobotToFreeplaySettings() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (null != robot) {
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingPets, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingOverheadEdges, true);
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
      _MiniGameInstance.SharedMinigameView.DialogCloseAnimationFinished -= HandleMinigameFinishedClosing;
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

    public override void CloseChallengeImmediately() {
      if (_MiniGameInstance != null) {
        DeregisterMinigameEvents();
        _MiniGameInstance.CloseChallengeImmediately();
        _MiniGameInstance = null;
        UnloadMinigameAssetBundle();
        StartLoadHomeView();
      }
    }

    private void DeregisterMinigameEvents() {
      if (_MiniGameInstance != null) {
        _MiniGameInstance.OnChallengeQuit -= HandleMiniGameQuit;
        _MiniGameInstance.OnChallengeWin -= HandleMiniGameWin;
        _MiniGameInstance.OnChallengeLose -= HandleMiniGameLose;
        _MiniGameInstance.OnShowEndGameDialog -= HandleEndGameDialog;
        _MiniGameInstance.OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
        if (_MiniGameInstance.SharedMinigameView != null) {
          _MiniGameInstance.SharedMinigameView.DialogCloseAnimationFinished -= HandleMinigameFinishedClosing;
        }
      }
    }

    private void CloseHomeView() {
      if (_ChallengeDetailsModalInstance != null) {
        _ChallengeDetailsModalInstance.DialogCloseAnimationFinished += HandleCloseDetailsDialogAnimationFinished;
        _ChallengeDetailsModalInstance.CloseDialog();
      }

      if (_HomeViewInstance != null) {
        DeregisterDialogEvents();
        _HomeViewInstance.DialogCloseAnimationFinished += HandleHomeViewCloseAnimationFinished;
        _HomeViewInstance.CloseDialog();
      }
    }

    private void HandleCloseDetailsDialogAnimationFinished() {
      _ChallengeDetailsModalInstance.DialogCloseAnimationFinished -= HandleCloseDetailsDialogAnimationFinished;
      _ChallengeDetailsModalInstance = null;
    }

    private void DeregisterDialogEvents() {
      if (_HomeViewInstance != null) {
        _HomeViewInstance.OnUnlockedChallengeClicked -= HandleUnlockedChallengeClicked;
        RequestGameManager.Instance.OnRequestGameConfirmed -= HandleStartChallengeRequest;
      }
    }

    // Every _kConnectedTimeIntervalCheck seconds, fire the OnConnectedInterval event for designer goals
    protected void Update() {
      if (Time.time - _ConnectedTimeIntervalLastTimestamp > _kConnectedTimeIntervalCheck) {
        _ConnectedTimeIntervalLastTimestamp = Time.time;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnConnectedInterval, Time.time - _ConnectedTimeStartedTimestamp));
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
        _HomeViewInstance.CloseDialogImmediately();
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
