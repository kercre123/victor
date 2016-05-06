using UnityEngine;
using System.Collections;

public class PressHubWorld : HubWorldBase {

  private GameBase _MiniGameInstance;

  [SerializeField]
  private ChallengeData _SpeedTapChallengeData;

  [SerializeField]
  private ChallengeData _FaceEnrollmentChallengeData;

  [SerializeField]
  private PressDemoView _PressDemoViewPrefab;
  private PressDemoView _PressDemoViewInstance;

  private bool _ForceProgressWhenOver = false;

  public override bool LoadHubWorld() {
    DebugMenuManager.Instance.EnableLatencyPopup(false);
    LoadPressDemoView();

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.OnRequestGameStart += HandleRequestSpeedTap;
    RobotEngineManager.Instance.OnRequestEnrollFace += HandleRequestEnrollFace;
    LoopRobotSleep();
    return true;
  }

  public override bool DestroyHubWorld() {
    GameObject.Destroy(_PressDemoViewInstance.gameObject);
    RobotEngineManager.Instance.OnRequestGameStart -= HandleRequestSpeedTap;
    RobotEngineManager.Instance.OnRequestEnrollFace -= HandleRequestEnrollFace;
    return true;
  }

  private void LoadPressDemoView() {
    _PressDemoViewInstance = UIManager.OpenView(_PressDemoViewPrefab);
    _PressDemoViewInstance.OnForceProgress += HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton += HandleStartButtonPressed;
  }

  private void HandleRequestEnrollFace(Anki.Cozmo.ExternalInterface.RequestEnrollFace message) {
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartFaceEnrollmentActivity);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
    alertView.TitleLocKey = "#pressDemo.faceEnrollTitle";
    alertView.DescriptionLocKey = "#pressDemo.faceEnrollDesc";
  }

  private void HandleRequestSpeedTap(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, StartSpeedTapGame);
    alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleRejection);
    alertView.TitleLocKey = "#pressDemo.speedTapTitle";
    alertView.DescriptionLocKey = "#pressDemo.speedTapDesc";
  }

  private void HandleStartButtonPressed(bool startWithEdge) {
    RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleSleepAnimationComplete);
  }

  private void HandleForceProgressPressed() {
    DAS.Debug(this, "Force Progress Pressed");
    RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
  }

  private void HandleRejection() {
    RobotEngineManager.Instance.SendDenyGameStart();
  }

  private void StartFaceEnrollmentActivity() {
    DAS.Debug(this, "Starting Face Enrollment Activity");
    PlayMinigame(_FaceEnrollmentChallengeData, false);
  }

  private void StartSpeedTapGame() {
    DAS.Debug(this, "Starting Speed Tap Game");
    PlayMinigame(_SpeedTapChallengeData, true);
  }

  private void PlayMinigame(ChallengeData challengeData, bool forceProgressWhenOver) {
    _ForceProgressWhenOver = forceProgressWhenOver;

    _PressDemoViewInstance.OnForceProgress -= HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton -= HandleStartButtonPressed;
    UIManager.CloseView(_PressDemoViewInstance);

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

    GameObject newMiniGameObject = GameObject.Instantiate(challengeData.MinigamePrefab);
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.InitializeMinigame(challengeData);
    _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
    _MiniGameInstance.OnMiniGameWin += HandleMinigameOver;
    _MiniGameInstance.OnMiniGameLose += HandleMinigameOver;
  }

  private void HandleMinigameOver(Transform[] rewards) {
    HandleMiniGameQuit();
  }

  private void HandleMiniGameQuit() {
    DAS.Debug(this, "activity ended so force transitioning to the next thing");
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    if (_ForceProgressWhenOver) {
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
