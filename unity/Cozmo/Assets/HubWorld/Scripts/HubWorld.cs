using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HubWorld : HubWorldBase {

  [SerializeField]
  private HubWorldDialog _HubWorldDialogPrefab;
  private HubWorldDialog _HubWorldDialogInstance;

  private List<ChallengeData> _ChallengeList = new List<ChallengeData>();

  private GameBase _MiniGameInstance;

  private List<string> _UnlockedLevels = new List<string>();

  [SerializeField]
  private TextAsset _TempLevelAsset;

  public override bool LoadHubWorld() {
    LoadChallengesJSON();
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

  private void LoadChallengesJSON() {
    _ChallengeList.Add(ParseChallengeJSON(_TempLevelAsset.ToString()));
  }

  private ChallengeData ParseChallengeJSON(string challengeJSON) {
    ChallengeData challengeData = new ChallengeData();

    JSONObject challengeDataObject = new JSONObject(challengeJSON);
    Debug.Assert(challengeDataObject.IsObject);
    challengeData.MinigamePrefabPath = challengeDataObject.GetField("MinigamePrefabPath").str;
    challengeData.ChallengeID = challengeDataObject.GetField("ChallengeID").str;
    challengeData.ChallengeTitleKey = challengeDataObject.GetField("ChallengeTitleKey").str;

    challengeData.ChallengeReqs = new ChallengeRequirements();

    JSONObject levelLockListData = challengeDataObject.GetField("ChallengeRequirements").GetField("LevelLocks");
    JSONObject statLockListData = challengeDataObject.GetField("ChallengeRequirements").GetField("StatLocks");

    for (int i = 0; i < levelLockListData.list.Count; ++i) {
      challengeData.ChallengeReqs.LevelLocks.Add(levelLockListData[i].str);
    }

    for (int i = 0; i < statLockListData.list.Count; ++i) {
      JSONObject statLockObject = statLockListData.list[i];
      challengeData.ChallengeReqs.StatLocks.Add(statLockObject.keys[0], (uint)statLockObject.i);
    }

    challengeData.MinigameParametersJSON = challengeDataObject.GetField("MinigameParametersJSON").ToString();

    return challengeData;
  }

  private void ShowHubWorldDialog() {
    // Create dialog with the game prefabs
    _HubWorldDialogInstance = UIManager.OpenDialog(_HubWorldDialogPrefab) as HubWorldDialog;
    _HubWorldDialogInstance.OnButtonClicked += OnButtonClicked;
    _HubWorldDialogInstance.Initialize(_ChallengeList);
  }

  private void OnButtonClicked(ChallengeData challengeClicked) {
    // don't load the level if the level doesn't meet requirements
    if (!challengeClicked.ChallengeReqs.MeetsRequirements(RobotEngineManager.Instance.CurrentRobot, _UnlockedLevels)) {
      return;
    }

    _HubWorldDialogInstance.OnButtonClicked -= OnButtonClicked;
    _HubWorldDialogInstance.CloseDialog();

    GameObject newMiniGameObject = GameObject.Instantiate(Resources.Load(challengeClicked.MinigamePrefabPath)) as GameObject;
    _MiniGameInstance = newMiniGameObject.GetComponent<GameBase>();
    _MiniGameInstance.ParseMinigameParams(challengeClicked.MinigameParametersJSON);
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
