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

    _PressDemoViewInstance = UIManager.OpenView(_PressDemoViewPrefab, verticalCanvas: true);
    _PressDemoViewInstance.OnForceProgress += HandleForceProgressPressed;

    StartSpeedTapGame();
    return true;
  }

  public override bool DestroyHubWorld() {
    GameObject.Destroy(_PressDemoViewInstance.gameObject);
    return true;
  }

  private void HandleForceProgressPressed() {
    
  }

  private void StartFaceEnrollmentActivity() {
    PlayMinigame(_FaceEnrollmentChallengeData);
  }

  private void StartSpeedTapGame() {
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
    
  }

}
