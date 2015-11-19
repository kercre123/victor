using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HubWorld : HubWorldBase {

  [SerializeField]
  private HubWorldDialog _HubWorldDialogPrefab;
  private HubWorldDialog _HubWorldDialogInstance;

  private GameBase _MiniGameInstance;

  private List<string> _UnlockedChallenges = new List<string>();

  [SerializeField]
  private TextAsset _TempLevelAsset;

  [SerializeField]
  private ChallengeDataList _ChallengeDataList;

  public override bool LoadHubWorld() {
    ShowHubWorldDialog();
    return true;
  }

  public override bool DestroyHubWorld() {
    // Deregister events
    // Destroy dialog if it exists
    if (_HubWorldDialogInstance != null) {
      _HubWorldDialogInstance.OnButtonClicked -= OnButtonClicked;
      _HubWorldDialogInstance.CloseDialogImmediately();
    }

    CloseMiniGame();
    return true;
  }
    
  private void ShowHubWorldDialog() {
    // Create dialog with the game prefabs
    _HubWorldDialogInstance = UIManager.OpenDialog(_HubWorldDialogPrefab) as HubWorldDialog;
    _HubWorldDialogInstance.OnButtonClicked += OnButtonClicked;
    _HubWorldDialogInstance.Initialize(_ChallengeDataList.ChallengeData);
  }

  private void OnButtonClicked(ChallengeData challengeClicked) {
    // don't load the level if the level doesn't meet requirements
    if (!challengeClicked.ChallengeReqs.MeetsRequirements(RobotEngineManager.Instance.CurrentRobot, _UnlockedChallenges)) {
      DAS.Info(this, "Level Requirements not met");
      return;
    }

    _HubWorldDialogInstance.OnButtonClicked -= OnButtonClicked;
    _HubWorldDialogInstance.CloseDialog();

    GameObject newMiniGameObject = GameObject.Instantiate(challengeClicked.MinigamePrefab);
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.LoadMinigameConfig(challengeClicked.MinigameConfig);
    _MiniGameInstance.OnMiniGameQuit += HandleMiniGameQuit;
  }

  private void HandleMiniGameQuit() {
    CloseMiniGame();
    ShowHubWorldDialog();
  }

  private void CloseMiniGame() {
    // Destroy game if it exists
    if (_MiniGameInstance != null) {
      _MiniGameInstance.CleanUp();
      Destroy(_MiniGameInstance.gameObject);
    }
  }

}
