using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

public class CozmoUnlocksPanel : MonoBehaviour {

  private List<UnlockableInfo> _Unlocked;
  private List<UnlockableInfo> _Available;
  private List<UnlockableInfo> _Unavailable;

  [SerializeField]
  private RectTransform _UnlocksContainer;

  [SerializeField]
  private GameObject _UnlocksTilePrefab;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _SparksLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _HexLabel;

  [SerializeField]
  private RequestTricksView _RequestSparkViewPrefab;
  private RequestTricksView _RequestSparkViewInstance;

  void Start() { 
    LoadTiles();
    RobotEngineManager.Instance.OnRequestSetUnlockResult += HandleRequestSetUnlockResult;
    UpdateInventoryCounts();
  }

  public void UpdateInventoryCounts() {
    _SparksLabel.FormattingArgs = new object[] { DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount };
    _HexLabel.FormattingArgs = new object[] { DataPersistenceManager.Instance.Data.DefaultProfile.HexPieces };
  }

  void OnDestroy() {
    RobotEngineManager.Instance.OnRequestSetUnlockResult -= HandleRequestSetUnlockResult;
  }

  public void LoadTiles() {

    ClearTiles();

    _Unlocked = UnlockablesManager.Instance.GetUnlocked(true);
    _Available = UnlockablesManager.Instance.GetAvailableAndLockedExplicit();
    _Unavailable = UnlockablesManager.Instance.GetUnavailableExplicit();

    string tileName;
    string viewControllerName = "home_hub_cozmo_unlock_panel";
    for (int i = 0; i < _Unlocked.Count; ++i) {
      GameObject tileInstance = GameObject.Instantiate(_UnlocksTilePrefab);
      tileInstance.transform.SetParent(_UnlocksContainer, false);
      tileInstance.name = _Unlocked[i].Id.Value.ToString();
      tileName = tileInstance.name + "\n(unlocked)";
      tileInstance.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = tileName;
      int unlockIndex = i;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().Initialize(() => HandleTappedUnlocked(_Unlocked[unlockIndex]),
        tileName, viewControllerName);
    }

    for (int i = 0; i < _Available.Count; ++i) {
      GameObject tileInstance = GameObject.Instantiate(_UnlocksTilePrefab);
      tileInstance.transform.SetParent(_UnlocksContainer, false);
      tileInstance.name = _Available[i].Id.Value.ToString();
      tileName = tileInstance.name + "\n(available)";
      tileInstance.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = tileName;
      int unlockIndex = i;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().Initialize(() => HandleTappedAvailable(_Available[unlockIndex]),
        tileName, viewControllerName);
    }

    for (int i = 0; i < _Unavailable.Count; ++i) {
      GameObject tileInstance = GameObject.Instantiate(_UnlocksTilePrefab);
      tileInstance.transform.SetParent(_UnlocksContainer, false);
      tileInstance.name = _Unavailable[i].Id.Value.ToString();
      tileName = tileInstance.name + "\n(locked)";
      tileInstance.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = tileName;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().Initialize(null,
        tileName, viewControllerName);
    }
  }

  private void ClearTiles() {
    for (int i = 0; i < _UnlocksContainer.transform.childCount; ++i) {
      GameObject.Destroy(_UnlocksContainer.transform.GetChild(i).gameObject);
    }
  }

  private void HandleTappedUnlocked(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped Unlocked: " + unlockInfo.Id);
    if (unlockInfo.UnlockableType == UnlockableType.Action) {
      _RequestSparkViewInstance = UIManager.OpenView<RequestTricksView>(_RequestSparkViewPrefab, verticalCanvas: true);
      _RequestSparkViewInstance.Initialize(unlockInfo, this);
    }
    else if (unlockInfo.UnlockableType == UnlockableType.Game) {
      // TODO: run the game that was unlocked.
    }
  }

  private void HandleTappedAvailable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    // TODO: This const should be replaced by the actual hex puzzle system when that's done.
    int unlockCost = 2;

    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.HexPieces < unlockCost) {
      return;
    }
    DataPersistenceManager.Instance.Data.DefaultProfile.HexPieces -= unlockCost;
    UpdateInventoryCounts();
    UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
  }

  private void HandleRequestSetUnlockResult(Anki.Cozmo.UnlockId unlockId, bool unlocked) {
    LoadTiles();
  }
}
