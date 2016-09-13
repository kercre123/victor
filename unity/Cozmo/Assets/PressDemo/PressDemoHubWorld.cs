using UnityEngine;
using System.Collections;
using Anki.Cozmo.ExternalInterface;

/// <summary>
///  handles scene transitions on the unity side for the press demo
///  scene here refers to the press demo scenes handled in the engine
///  not unity scenes.
/// </summary>
public class PressDemoHubWorld : HubWorldBase {

  private GameBase _MiniGameInstance;

  [SerializeField]
  private ChallengeData _SpeedTapChallengeData;

  [SerializeField]
  private ChallengeData _FaceEnrollmentChallengeData;

  [SerializeField]
  private PressDemoView _PressDemoViewPrefab;
  private PressDemoView _PressDemoViewInstance;

  private bool _SpeedTapEndLogic;

  private int _PressDemoDebugSceneIndex = 0;

  private int _ExplicitFaceID = -1;

  private Cozmo.UI.AlertView _RequestDialog = null;

  private void SetDemoStartValues() {
    _SpeedTapEndLogic = false;
    _PressDemoDebugSceneIndex = 0;
    _ExplicitFaceID = -1;
  }

  private void Awake() {
    RobotEngineManager.Instance.CurrentRobot.LoadFaceAlbumFromFile("prDemo", true);
    SkillSystem.Instance.DebugEraseStorage();
    HackSkillSpeedTapSetup();
    // force unlock face enrollment
    RobotEngineManager.Instance.CurrentRobot.RequestSetUnlock(Anki.Cozmo.UnlockId.MeetCozmoGame, true);
  }

  // HACK: demo hacked settings for skill level selection
  private void HackSkillSpeedTapSetup() {
    DataPersistence.PlayerProfile playerProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
    playerProfile.GameDifficulty["SpeedTapGame"] = 1;
    DataPersistence.DataPersistenceManager.Instance.Save();
  }

  public override void LoadHubWorld() {
    DebugMenuManager.Instance.EnableLatencyPopup(false);
    LoadPressDemoView();

    RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.AddCallback<RequestGameStart>(HandleRequestSpeedTap);
    RobotEngineManager.Instance.AddCallback<RequestEnrollFace>(HandleRequestEnrollFace);
    RobotEngineManager.Instance.AddCallback<DemoState>(HandleDemoState);

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);

    RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.StartSleeping, HandleSleepAnimationComplete);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Sleep);
  }

  public override void DestroyHubWorld() {
    GameObject.Destroy(_PressDemoViewInstance.gameObject);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleRequestSpeedTap);
    RobotEngineManager.Instance.RemoveCallback<RequestEnrollFace>(HandleRequestEnrollFace);
    RobotEngineManager.Instance.RemoveCallback<DemoState>(HandleDemoState);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);
    GameObject.Destroy(this.gameObject);
  }

  private void LoadPressDemoView() {
    _PressDemoViewInstance = UIManager.OpenView(_PressDemoViewPrefab);
    _PressDemoViewInstance.OnForceProgress += HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton += HandleStartButtonPressed;
    _PressDemoViewInstance.SetPressDemoDebugState(_PressDemoDebugSceneIndex);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
  }

  private void HandleDemoState(DemoState demoStateMessage) {
    int demoNum = demoStateMessage.stateNum;
    _PressDemoDebugSceneIndex = demoNum;
    DAS.Debug("PressDemo", "Demo State #: " + demoNum);
    if (_PressDemoViewInstance != null) {
      _PressDemoViewInstance.SetPressDemoDebugState(demoNum);
    }
    // last demo scene so shut off music
    if (demoNum == 7) {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      _PressDemoViewInstance.HideStartButtons(false, "Restart");
    }
  }

  private void HandleRequestEnrollFace(Anki.Cozmo.ExternalInterface.RequestEnrollFace message) {
    DAS.Info("PressDemoHubWorld.HandleRequestEnrollFace", "Engine Requested Face Enroll");
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetIcon(_FaceEnrollmentChallengeData.ChallengeIcon);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartFaceEnrollmentActivity);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
    alertView.TitleLocKey = "pressDemo.faceEnrollRequestTitle";
    alertView.DescriptionLocKey = "pressDemo.faceEnrollRequestDesc";
    _RequestDialog = alertView;
    _ExplicitFaceID = message.face_id;
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Request_Game);
  }

  private void HandleExternalRejection(object message) {
    DAS.Info(this, "PressDemoHubWorld.HandleExternalRejection");
    if (_RequestDialog != null) {
      _RequestDialog.CloseView();
      _RequestDialog = null;
    }
  }

  private void HandleRequestSpeedTap(RequestGameStart message) {
    DAS.Info("PressDemoHubWorld.HandleRequestSpeedTap", "Engine Requested Speed Tap");
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetIcon(_SpeedTapChallengeData.ChallengeIcon);

    if (message.firstRequest) {
      alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartSpeedTapGame);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
      alertView.TitleLocKey = LocalizationKeys.kPressDemoSpeedTapRequestTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kPressDemoSpeedTapRequestDesc;
    }
    else {
      alertView.SetPrimaryButton(LocalizationKeys.kPressDemoYesSecondTime, StartSpeedTapGame);
      alertView.SetSecondaryButton(LocalizationKeys.kPressDemoNoSecondTime, HandleRejection);
      alertView.TitleLocKey = LocalizationKeys.kPressDemoSpeedTapRequestAgainTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kPressDemoSpeedTapRequestAgainDesc;
    }
    _RequestDialog = alertView;
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Request_Game);
  }

  private void HandleStartButtonPressed(bool startWithEdge) {
    SetDemoStartValues();
    RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleSleepAnimationComplete);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
  }

  private void HandleForceProgressPressed() {
    DAS.Info(this, "Force Progress Pressed");
    RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
  }

  private void HandleRejection() {
    _RequestDialog = null;
    RobotEngineManager.Instance.SendDenyGameStart();
  }

  private void StartFaceEnrollmentActivity() {
    DAS.Info(this, "Starting Face Enrollment Activity");
    _RequestDialog = null;
    // demo mode should not be saving faces to the actual robot.
    bool playGameSpecificMusic = false;
    PlayMinigame(_FaceEnrollmentChallengeData, playGameSpecificMusic,
                 (GameBase game) => {
                   FaceEnrollment.FaceEnrollmentGame faceEnrollment = game as FaceEnrollment.FaceEnrollmentGame;
                   faceEnrollment.SetSaveToRobot(false);
                   faceEnrollment.SetFixedFaceID(_ExplicitFaceID);
                 });
  }

  private void HandleSpeedTapYesAnimationEnd(bool success) {
    DAS.Info(this, "Starting Speed Tap Game");
    PlayMinigame(_SpeedTapChallengeData, playGameSpecificMusic: true);
  }

  private void StartSpeedTapGame() {
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    _SpeedTapEndLogic = true;

    RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSpeedtapCozmoConfirm, HandleSpeedTapYesAnimationEnd);
  }

  private void PlayMinigame(ChallengeData challengeData, bool playGameSpecificMusic,
                            System.Action<GameBase> gameFinishedLoadingCallback = null) {

    _PressDemoViewInstance.OnForceProgress -= HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton -= HandleStartButtonPressed;
    UIManager.CloseView(_PressDemoViewInstance);

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

    Anki.Assets.AssetBundleManager.Instance.LoadAssetBundleAsync(
      challengeData.PrefabDataAssetBundle, (bool success) => {
        challengeData.LoadPrefabData((ChallengePrefabData prefabData) => {
          GameObject newMiniGameObject = Instantiate(prefabData.MinigamePrefab);
          _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
          _MiniGameInstance.InitializeMinigame(challengeData, playGameSpecificMusic);
          _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
          _MiniGameInstance.OnMiniGameWin += HandleMinigameOver;
          _MiniGameInstance.OnMiniGameLose += HandleMinigameOver;
          _MiniGameInstance.OnShowEndGameDialog += HandleEndGameDialog;

          if (gameFinishedLoadingCallback != null) {
            gameFinishedLoadingCallback(_MiniGameInstance);
          }
        });
      });
  }

  private void HandleEndGameDialog() {
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.NoGame);
    RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
    RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
    RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
    // TODO : Remove this once we have a more stable, permanent solution in Engine for false cliff detection
    RobotEngineManager.Instance.CurrentRobot.SetEnableCliffSensor(true);
  }

  private void HandleMinigameOver() {
    HandleMiniGameQuit();
  }

  private void HandleMiniGameQuit() {
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);

    LoadPressDemoView();

    if (_SpeedTapEndLogic) {
      RequestSpeedTapLoop();
    }
    _PressDemoViewInstance.HideStartButtons();
    _PressDemoViewInstance.HideNoEdgeButton();
  }

  private void RequestSpeedTapLoop() {
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

    DAS.Info("PressDemoHubWorld.RequestSpeedTapLoop", "Request Speed Tap Loop");
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetIcon(_SpeedTapChallengeData.ChallengeIcon);

    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartSpeedTapGame);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleSpeedTapLoopEnd);
    alertView.DescriptionLocKey = LocalizationKeys.kPressDemoSpeedTapLoopText;
    _RequestDialog = alertView;
  }

  private void HandleSpeedTapLoopEnd() {
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);
  }

  private void LoopRobotSleep() {
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Info("PressHubWorld.LoopRobotSleep", "Sending Sleeping Animation");
      robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.Sleeping, HandleSleepAnimationComplete);
    }
  }

  private void HandleSleepAnimationComplete(bool success) {
    DAS.Info("PressHubWorld.HandleSleepAnimationComplete", "HandleSleepAnimationComplete: success: " + success);
    LoopRobotSleep();
  }

}
