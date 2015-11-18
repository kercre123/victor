using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HubWorld : HubWorldBase {

  [SerializeField]
  private HubWorldDialog _HubWorldDialogPrefab;
  private HubWorldDialog _HubWorldDialogInstance;

  private List<ChallengeData> _ChallengeList = new List<ChallengeData>();

  private GameBase _MiniGameInstance;

  private List<string> _UnlockedChallenges = new List<string>();

  [SerializeField]
  private TextAsset _TempLevelAsset;

  [SerializeField]
  private string _ChallengeListPath;

  public override bool LoadHubWorld() {
    LoadChallenges();
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

  private void LoadChallenges() {
    ChallengeDataList list = Resources.Load(_ChallengeListPath) as ChallengeDataList;

    for (int i = 0; i < list.ChallengeDataNames.Length; ++i) {
      string challengeFileName = list.ChallengeDataNames[i];
      _ChallengeList.Add(Resources.Load("Data/Challenges/" + challengeFileName) as ChallengeData);
    }
  }

  private void ShowHubWorldDialog() {
    // Create dialog with the game prefabs
    _HubWorldDialogInstance = UIManager.OpenDialog(_HubWorldDialogPrefab) as HubWorldDialog;
    _HubWorldDialogInstance.OnButtonClicked += OnButtonClicked;
    _HubWorldDialogInstance.Initialize(_ChallengeList);
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
    _MiniGameInstance.LoadMinigameConfig(challengeClicked.MinigameConfigPath);
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
