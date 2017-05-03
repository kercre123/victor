using Anki.Assets;
using Anki.Cozmo;
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
      // Destroy dialog if it exists
      if (_NeedsViewHubInstance != null) {
        DeregisterDialogEvents();

        // Since NeedsHubView is a BaseView, it will be closed the next time a view opens. 
        // This is only called during robot disconnect so it should be closed right away by
        // the opening of the NeedsUnconnectedView
        // _NeedsViewHubInstance.CloseDialogImmediately();
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

    private void StartLoadNeedsHubView() {
      LoadNeedsHubViewAssetBundle(LoadNeedsHubView);
    }

    private void LoadNeedsHubViewAssetBundle(System.Action<bool> loadCallback) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_NeedsHubViewPrefabData.AssetBundle, loadCallback);
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

      // Close dialog
      CloseNeedsHubView();
    }

    private void HandleNeedsHubViewCloseAnimationFinished() {
      if (_NeedsViewHubInstance != null) {
        _NeedsViewHubInstance.DialogCloseAnimationFinished -= HandleNeedsHubViewCloseAnimationFinished;
        _NeedsViewHubInstance = null;
      }
      AssetBundleManager.Instance.UnloadAssetBundle(_NeedsHubViewPrefabData.AssetBundle);

      if (!_ChallengeManager.LoadChallenge()) {
        StartLoadNeedsHubView();
      }
    }

    private void HandleChallengeFinishedLoading(AnimationTrigger getOutAnim) {
      _ChallengeGetOutAnimTrigger = getOutAnim;
      RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.Count);
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

    private void CloseNeedsHubView() {
      if (_NeedsViewHubInstance != null) {
        DeregisterDialogEvents();
        _NeedsViewHubInstance.DialogCloseAnimationFinished += HandleNeedsHubViewCloseAnimationFinished;
        _NeedsViewHubInstance.CloseDialog();
      }
    }

    private void DeregisterDialogEvents() {
      if (_NeedsViewHubInstance != null) {
        _NeedsViewHubInstance.OnStartChallengeClicked -= HandleStartRandomChallenge;
      }
    }

    // Every _kConnectedTimeIntervalCheck seconds, fire the OnConnectedInterval event for designer goals
    protected void Update() {
      if (Time.time - _ConnectedTimeIntervalLastTimestamp > _kConnectedTimeIntervalCheck) {
        _ConnectedTimeIntervalLastTimestamp = Time.time;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnConnectedInterval, Time.time - _ConnectedTimeStartedTimestamp));
      }
    }
  }
}
