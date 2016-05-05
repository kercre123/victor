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

    _PressDemoViewInstance = UIManager.OpenView(_PressDemoViewPrefab);
    _PressDemoViewInstance.OnForceProgress += HandleForceProgressPressed;
    _PressDemoViewInstance.OnStartButton += HandleStartButtonPressed;

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.OnRequestGameStart += StartSpeedTapGame;
    LoopRobotSleep();
    return true;
  }

  public override bool DestroyHubWorld() {
    GameObject.Destroy(_PressDemoViewInstance.gameObject);
    return true;
  }

  private void HandleStartButtonPressed(bool startWithEdge) {
    RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleSleepAnimationComplete);
  }

  private void HandleForceProgressPressed() {
    DAS.Debug(this, "Force Progress Pressed");
    RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
  }

  private void StartFaceEnrollmentActivity() {
    DAS.Debug(this, "Starting Face Enrollment Activity");
    PlayMinigame(_FaceEnrollmentChallengeData, false);
  }

  private void StartSpeedTapGame(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    DAS.Debug(this, "Starting Speed Tap Game");
    PlayMinigame(_SpeedTapChallengeData, true);
  }

  private void PlayMinigame(ChallengeData challengeData, bool forceProgressWhenOver) {
    _ForceProgressWhenOver = forceProgressWhenOver;

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
