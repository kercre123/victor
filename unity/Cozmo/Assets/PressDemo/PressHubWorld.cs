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

  public override bool LoadHubWorld() {

    _PressDemoViewInstance = UIManager.OpenView(_PressDemoViewPrefab);
    _PressDemoViewInstance.OnForceProgress += HandleForceProgressPressed;

    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Demo);
    RobotEngineManager.Instance.OnRequestGameStart += StartSpeedTapGame;

    return true;
  }

  public override bool DestroyHubWorld() {
    GameObject.Destroy(_PressDemoViewInstance.gameObject);
    return true;
  }

  private void HandleForceProgressPressed() {
    DAS.Debug(this, "Force Progress Pressed");
    RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
  }

  private void StartFaceEnrollmentActivity() {
    DAS.Debug(this, "Starting Face Enrollment Activity");
    PlayMinigame(_FaceEnrollmentChallengeData);
  }

  private void StartSpeedTapGame(Anki.Cozmo.ExternalInterface.RequestGameStart message) {
    DAS.Debug(this, "Starting Speed Tap Game");
    PlayMinigame(_SpeedTapChallengeData);
  }

  private void PlayMinigame(ChallengeData challengeData) {
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
    RobotEngineManager.Instance.CurrentRobot.TransitionToNextDemoState();
  }

}
