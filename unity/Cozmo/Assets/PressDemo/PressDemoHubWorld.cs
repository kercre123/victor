using UnityEngine;
using System.Collections;

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

  private bool _ProgressSceneWhenMinigameOver = false;

  private int _PressDemoDebugSceneIndex = 0;

  private int _ExplicitFaceID = -1;

  private Cozmo.UI.AlertView _RequestDialog = null;

  public override bool LoadHubWorld() {
    DebugMenuManager.Instance.EnableLatencyPopup(false);
    LoadPressDemoView();

    RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.OnRequestGameStart += HandleRequestSpeedTap;
    RobotEngineManager.Instance.OnRequestEnrollFace += HandleRequestEnrollFace;
    RobotEngineManager.Instance.OnDemoState += HandleDemoState;
    RobotEngineManager.Instance.OnDenyGameStart += HandleExternalRejection;
    RobotEngineManager.Instance.CurrentRobot.SendAnimation(AnimationName.kStartSleeping, HandleSleepAnimationComplete);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Sleep);
    return true;
  }

  public override bool DestroyHubWorld() {
    GameObject.Destroy(_PressDemoViewInstance.gameObject);
    RobotEngineManager.Instance.OnRequestGameStart -= HandleRequestSpeedTap;
    RobotEngineManager.Instance.OnRequestEnrollFace -= HandleRequestEnrollFace;
    RobotEngineManager.Instance.OnDemoState -= HandleDemoState;
    RobotEngineManager.Instance.OnDenyGameStart -= HandleExternalRejection;
    return true;
  }

  private void LoadPressDemoView() {
    _PressDemoViewInstance = UIManager.OpenView(_PressDemoViewPrefab);
    _PressDemoViewInstance.OnForceProgress += HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton += HandleStartButtonPressed;
    _PressDemoViewInstance.SetPressDemoDebugState(_PressDemoDebugSceneIndex);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Hub);
  }

  private void HandleDemoState(int demoNum) {
    _PressDemoDebugSceneIndex = demoNum;
    if (_PressDemoViewInstance != null) {
      _PressDemoViewInstance.SetPressDemoDebugState(demoNum);
    }
  }

  private void HandleRequestEnrollFace(Anki.Cozmo.ExternalInterface.RequestEnrollFace message) {
    DAS.Debug("PressDemoHubWorld.HandleRequestSpeedTap", "Engine Requested Face Enroll");
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetIcon(_FaceEnrollmentChallengeData.ChallengeIcon);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartFaceEnrollmentActivity);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
    alertView.TitleLocKey = "pressDemo.faceEnrollRequestTitle";
    alertView.DescriptionLocKey = "pressDemo.faceEnrollRequestDesc";
    _RequestDialog = alertView;
    _ExplicitFaceID = message.face_id;
  }

  private void HandleExternalRejection(Anki.Cozmo.ExternalInterface.DenyGameStart message) {
    DAS.Info(this, "PressDemoHubWorld.HandleExternalRejection");
    if (_RequestDialog != null) {
      _RequestDialog.CloseView();
      _RequestDialog = null;
    }
  }

  private void HandleRequestSpeedTap(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    DAS.Debug("PressDemoHubWorld.HandleRequestSpeedTap", "Engine Requested Speed Tap");
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetIcon(_SpeedTapChallengeData.ChallengeIcon);

    if (message.firstRequest) {
      alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartSpeedTapGame);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
      alertView.TitleLocKey = "pressDemo.speedTapRequestTitle";
      alertView.DescriptionLocKey = "pressDemo.speedTapRequestDesc";
    }
    else {
      alertView.SetPrimaryButton("pressDemo.ohAlright", StartSpeedTapGame);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
      alertView.TitleLocKey = "pressDemo.speedTapRequestAgainTitle";
      alertView.DescriptionLocKey = "pressDemo.speedTapRequestAgainDesc";
    }
    _RequestDialog = alertView;
  }

  private void HandleStartButtonPressed(bool startWithEdge) {
    RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleSleepAnimationComplete);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Hub);
  }

  private void HandleForceProgressPressed() {
    DAS.Debug(this, "Force Progress Pressed");
    RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
  }

  private void HandleRejection() {
    _RequestDialog = null;
    RobotEngineManager.Instance.SendDenyGameStart();
  }

  private void StartFaceEnrollmentActivity() {
    DAS.Debug(this, "Starting Face Enrollment Activity");
    FaceEnrollment.FaceEnrollmentGame faceEnrollment = PlayMinigame(_FaceEnrollmentChallengeData, progressSceneWhenMinigameOver: false, playGameSpecificMusic: false) as FaceEnrollment.FaceEnrollmentGame;
    _RequestDialog = null;
    // demo mode should not be saving faces to the actual robot.
    faceEnrollment.SetSaveToRobot(false);
    faceEnrollment.SetFixedFaceID(_ExplicitFaceID);
  }

  private void StartSpeedTapGame() {
    DAS.Debug(this, "Starting Speed Tap Game");
    PlayMinigame(_SpeedTapChallengeData, progressSceneWhenMinigameOver: false, playGameSpecificMusic: true);
    int maxLevel = _SpeedTapChallengeData.MinigameConfig.SkillConfig.GetMaxLevel();
    SkillSystem.Instance.SetDebugSkillsForGame(maxLevel, maxLevel, maxLevel);
  }

  private GameBase PlayMinigame(ChallengeData challengeData, bool progressSceneWhenMinigameOver, bool playGameSpecificMusic) {
    _ProgressSceneWhenMinigameOver = progressSceneWhenMinigameOver;

    _PressDemoViewInstance.OnForceProgress -= HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton -= HandleStartButtonPressed;
    UIManager.CloseView(_PressDemoViewInstance);

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

    GameObject newMiniGameObject = GameObject.Instantiate(challengeData.MinigamePrefab);
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.InitializeMinigame(challengeData, playGameSpecificMusic);
    _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
    _MiniGameInstance.OnMiniGameWin += HandleMinigameOver;
    _MiniGameInstance.OnMiniGameLose += HandleMinigameOver;
    _MiniGameInstance.OnShowEndGameDialog += HandleEndGameDialog;
    return _MiniGameInstance;
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

  private void HandleMinigameOver(Transform[] rewards) {
    HandleMiniGameQuit();
  }

  private void HandleMiniGameQuit() {
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);
    if (_ProgressSceneWhenMinigameOver) {
      RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
    }
    LoadPressDemoView();
    _PressDemoViewInstance.HideStartButtons();
  }

  private void LoopRobotSleep() {
    var robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      DAS.Info("PressHubWorld.LoopRobotSleep", "Sending Sleeping Animation");
      robot.SendAnimation(AnimationName.kSleeping, HandleSleepAnimationComplete);
    }
  }

  private void HandleSleepAnimationComplete(bool success) {
    DAS.Info("PressHubWorld.HandleSleepAnimationComplete", "HandleSleepAnimationComplete: success: " + success);
    LoopRobotSleep();
  }

}
