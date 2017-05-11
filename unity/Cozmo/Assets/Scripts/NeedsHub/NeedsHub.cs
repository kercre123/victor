using Anki.Assets;
using Anki.Cozmo;
using Cozmo.Needs.Activities.UI;
using Cozmo.Challenge;
using Cozmo.Needs.UI;
using Cozmo.Util;
using DataPersistence;
using System.Collections;
using UnityEngine;

namespace Cozmo.Hub {
  public class NeedsHub : HubWorldBase {

    [SerializeField]
    private SerializableAssetBundleNames _ChallengeDataPrefabAssetBundle;

    [SerializeField]
    private GameObjectDataLink _NeedsHubViewPrefabData;

    private NeedsHubView _NeedsViewHubInstance;

    [SerializeField]
    private GameObjectDataLink _ActivitiesViewPrefabData;

    private ActivitiesView _ActivitiesViewInstance;

    private AnimationTrigger _ChallengeGetOutAnimTrigger = AnimationTrigger.Count;

    private ChallengeManager _ChallengeManager;

    // Total ConnectedTime For GameEvents
    private const float _kConnectedTimeIntervalCheck = 60.0f;
    private float _ConnectedTimeIntervalLastTimestamp = -1;
    private float _ConnectedTimeStartedTimestamp = -1;

    public override void LoadHubWorld() {
      _ChallengeManager = new ChallengeManager(_ChallengeDataPrefabAssetBundle);
      _ChallengeManager.OnShowEndGameDialog += HandleEndGameDialog;
      _ChallengeManager.OnChallengeViewFinishedClosing += StartLoadNeedsHubView;
      _ChallengeManager.OnChallengeCompleted += HandleChallengeComplete;
      _ChallengeManager.OnChallengeFinishedLoading += HandleChallengeFinishedLoading;

      _Instance = this;
      StartLoadNeedsHubView();

      _ConnectedTimeStartedTimestamp = Time.time;
      _ConnectedTimeIntervalLastTimestamp = _ConnectedTimeStartedTimestamp;
    }

    public override void DestroyHubWorld() {
      _ChallengeManager.CleanUp();
      _ChallengeManager.OnShowEndGameDialog -= HandleEndGameDialog;
      _ChallengeManager.OnChallengeViewFinishedClosing -= StartLoadNeedsHubView;
      _ChallengeManager.OnChallengeCompleted -= HandleChallengeComplete;
      _ChallengeManager.OnChallengeFinishedLoading -= HandleChallengeFinishedLoading;

      // Deregister events
      if (_NeedsViewHubInstance != null) {
        DeregisterNeedsViewEvents();

        // Since NeedsHubView is a BaseView, it will be closed the next time a view opens. 
        // This is only called during robot disconnect so it should be closed right away by
        // the opening of the NeedsUnconnectedView
        // _NeedsViewHubInstance.CloseDialogImmediately();
      }

      if (_ActivitiesViewInstance != null) {
        DeregisterActivitiesViewEvents();
      }

      // Destroy self
      GameObject.Destroy(this.gameObject);
    }

    public override GameBase GetChallengeInstance() {
      if (_ChallengeManager != null) {
        return _ChallengeManager.ChallengeInstance;
      }
      return null;
    }

    public override void CloseChallengeImmediately() {
      if (_ChallengeManager != null) {
        _ChallengeManager.CloseChallengeImmediately();
      }
    }

    #region LoadNeedsHub

    private void StartLoadNeedsHubView() {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_NeedsHubViewPrefabData.AssetBundle, LoadNeedsHubView);
    }

    private void LoadNeedsHubView(bool assetBundleSuccess) {
      if (assetBundleSuccess) {
        _NeedsHubViewPrefabData.LoadAssetData((GameObject needsHubViewPrefab) => {
          // this can be null if, by the time this callback is called, NeedsHub has been destroyed.
          // This seems to be happening in some disconnection cases and a NRE is thrown
          if (this != null) {
            if (needsHubViewPrefab != null) {
              StartCoroutine(ShowNeedsHubViewAfterOtherViewClosed(needsHubViewPrefab));
            }
            else {
              DAS.Error("NeedsHub.LoadNeedsHubView", "needsHubViewPrefab is null");
            }
          }
        });
      }
      else {
        DAS.Error("NeedsHub.LoadNeedsHubView", "Failed to load asset bundle " + _NeedsHubViewPrefabData.AssetBundle);
      }
    }

    private IEnumerator ShowNeedsHubViewAfterOtherViewClosed(GameObject needsHubViewPrefab) {
      // wait until challenge instance is destroyed and also wait until the unlocks are properly loaded from
      // robot.
      while (_ChallengeManager.IsChallengePlaying || !UnlockablesManager.Instance.UnlocksLoaded) {
        yield return 0;
      }

      UIManager.OpenView(needsHubViewPrefab.GetComponent<NeedsHubView>(), (newNeedsHubView) => {
        _NeedsViewHubInstance = (NeedsHubView)newNeedsHubView;
        _NeedsViewHubInstance.OnStartChallengeClicked += HandleStartRandomChallenge;
        _NeedsViewHubInstance.OnActivitiesButtonClicked += HandleActivitiesButtonClicked;

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
            robot.SendAnimationTrigger(_ChallengeGetOutAnimTrigger, (bool success) => {
              _ChallengeGetOutAnimTrigger = AnimationTrigger.Count;
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
        robot.SetEnableFreeplayBehaviorChooser(true);
      }
    }

    #endregion

    #region StartChallenge

    private void HandleStartRandomChallenge() {
      string randomChallengeId = _ChallengeManager.GetRandomChallenge();
      PlayChallenge(randomChallengeId, false);
    }

    private void PlayChallenge(string challengeId, bool wasRequest) {
      _ChallengeManager.SetCurrentChallenge(challengeId, wasRequest);

      // Reset the robot behavior
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
        // If accepted a request, because we've turned off freeplay behavior
        // we need to send Cozmo their animation from unity.
        RequestGameConfig rc = _ChallengeManager.GetCurrentRequestGameConfig();
        if (rc != null) {
          RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(rc.RequestAcceptedAnimationTrigger.Value);
        }
      }

      // Close needs dialog if open
      if (_NeedsViewHubInstance != null) {
        DeregisterNeedsViewEvents();
        _NeedsViewHubInstance.DialogCloseAnimationFinished += StartLoadChallenge;
        _NeedsViewHubInstance.CloseDialog();
      }

      if (_ActivitiesViewInstance != null) {
        DeregisterActivitiesViewEvents();
        _ActivitiesViewInstance.DialogCloseAnimationFinished += StartLoadChallenge;
        _ActivitiesViewInstance.CloseDialog();
      }
    }

    private void DeregisterNeedsViewEvents() {
      _NeedsViewHubInstance.OnStartChallengeClicked -= HandleStartRandomChallenge;
      _NeedsViewHubInstance.OnActivitiesButtonClicked -= HandleActivitiesButtonClicked;
      _NeedsViewHubInstance.DialogCloseAnimationFinished -= StartLoadChallenge;
    }

    private void StartLoadChallenge() {
      if (_NeedsViewHubInstance != null) {
        DeregisterNeedsViewEvents();
        _NeedsViewHubInstance = null;
      }
      AssetBundleManager.Instance.UnloadAssetBundle(_NeedsHubViewPrefabData.AssetBundle);

      if (_ActivitiesViewInstance != null) {
        DeregisterActivitiesViewEvents();
        _ActivitiesViewInstance = null;
      }
      AssetBundleManager.Instance.UnloadAssetBundle(_ActivitiesViewPrefabData.AssetBundle);

      if (!_ChallengeManager.LoadChallenge()) {
        StartLoadNeedsHubView();
      }
    }

    private void HandleChallengeFinishedLoading(AnimationTrigger getOutAnim) {
      _ChallengeGetOutAnimTrigger = getOutAnim;
      RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.Count);
    }

    #endregion

    #region HandleChallengeEnd

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
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingPets, true);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingOverheadEdges, true);
        // TODO : Remove this once we have a more stable, permanent solution in Engine for false cliff detection
        robot.SetEnableCliffSensor(true);
      }
    }

    private void HandleChallengeComplete() {
      // the last session is not necessarily valid as the 'CurrentSession', as its possible
      // the day rolled over while we were playing the challenge.
      var session = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault();
      if (session == null) {
        DAS.Error(this, "Somehow managed to complete a challenge with no sessions saved!");
      }

      DataPersistenceManager.Instance.Save();
    }

    #endregion

    #region OpenActivitiesView

    private void HandleActivitiesButtonClicked() {
      DeregisterNeedsViewEvents();
      // Start load activities view
      AssetBundleManager.Instance.LoadAssetBundleAsync(_ActivitiesViewPrefabData.AssetBundle, LoadActivitiesView);
    }

    private void LoadActivitiesView(bool assetBundleSuccess) {
      if (assetBundleSuccess) {
        _ActivitiesViewPrefabData.LoadAssetData((GameObject activitiesViewPrefab) => {
          // this can be null if, by the time this callback is called, NeedsHub has been destroyed.
          // This seems to be happening in some disconnection cases and a NRE is thrown
          if (this != null) {
            if (activitiesViewPrefab != null) {
              ShowActivitiesView(activitiesViewPrefab);
            }
            else {
              DAS.Error("NeedsHub.LoadActivitiesView", "activitiesViewPrefab is null");
            }
          }
        });
      }
      else {
        DAS.Error("NeedsHub.LoadActivitiesView", "Failed to load asset bundle " + _ActivitiesViewPrefabData.AssetBundle);
      }
    }

    private void ShowActivitiesView(GameObject activitiesViewPrefab) {
      UIManager.OpenView(activitiesViewPrefab.GetComponent<ActivitiesView>(), (newActivitiesView) => {
        _ActivitiesViewInstance = (ActivitiesView)newActivitiesView;
        _ActivitiesViewInstance.OnBackButtonPressed += HandleBackToNeedsPressed;
        _ActivitiesViewInstance.InitializeActivitiesView(_ChallengeManager.GetActivities());
        _ActivitiesViewInstance.OnActivityButtonPressed += HandleStartActivityPressed;
      });
    }

    private void HandleBackToNeedsPressed() {
      DeregisterActivitiesViewEvents();
      StartLoadNeedsHubView();
    }

    private void HandleStartActivityPressed(string challengeId) {
      PlayChallenge(challengeId, wasRequest: false);
    }

    private void DeregisterActivitiesViewEvents() {
      _ActivitiesViewInstance.OnBackButtonPressed -= HandleBackToNeedsPressed;
      _ActivitiesViewInstance.OnActivityButtonPressed -= HandleStartActivityPressed;
      _ActivitiesViewInstance.DialogCloseAnimationFinished -= StartLoadChallenge;
    }

    #endregion

    // Every _kConnectedTimeIntervalCheck seconds, fire the OnConnectedInterval event for designer goals
    protected void Update() {
      if (Time.time - _ConnectedTimeIntervalLastTimestamp > _kConnectedTimeIntervalCheck) {
        _ConnectedTimeIntervalLastTimestamp = Time.time;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnConnectedInterval, Time.time - _ConnectedTimeStartedTimestamp));
      }
    }
  }
}
