using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class CozmoUnlocksPanel : MonoBehaviour {

  private List<UnlockableInfo> _Unlocked;
  private List<UnlockableInfo> _Available;
  private List<UnlockableInfo> _Unavailable;

  [SerializeField]
  private RectTransform _UnlocksContainer;

  [SerializeField]
  private GameObject _UnlocksTilePrefab;

  void Start() { 
    LoadTiles();
    RobotEngineManager.Instance.OnRequestSetUnlockResult += HandleRequestSetUnlockResult;
  }

  public void LoadTiles() {

    ClearTiles();

    _Unlocked = UnlockablesManager.Instance.GetUnlocked(true);
    _Available = UnlockablesManager.Instance.GetAvailableAndLockedExplicit();
    _Unavailable = UnlockablesManager.Instance.GetUnavailableExplicit();

    for (int i = 0; i < _Unlocked.Count; ++i) {
      GameObject tileInstance = GameObject.Instantiate(_UnlocksTilePrefab);
      tileInstance.transform.SetParent(_UnlocksContainer, false);
      tileInstance.name = _Unlocked[i].Id.Value.ToString();
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().DASEventButtonName = tileInstance.name;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().DASEventViewController = "cozmo_unlock_panel";
      tileInstance.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = tileInstance.name + "\n(unlocked)";
      int unlockIndex = i;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().onClick.AddListener(() => HandleTappedUnlocked(_Unlocked[unlockIndex]));
    }

    for (int i = 0; i < _Available.Count; ++i) {
      GameObject tileInstance = GameObject.Instantiate(_UnlocksTilePrefab);
      tileInstance.transform.SetParent(_UnlocksContainer, false);
      tileInstance.name = _Available[i].Id.Value.ToString();
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().DASEventButtonName = tileInstance.name;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().DASEventViewController = "cozmo_unlock_panel";
      tileInstance.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = tileInstance.name + "\n(available)";
      int unlockIndex = i;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().onClick.AddListener(() => HandleTappedAvailable(_Available[unlockIndex]));
    }

    for (int i = 0; i < _Unavailable.Count; ++i) {
      GameObject tileInstance = GameObject.Instantiate(_UnlocksTilePrefab);
      tileInstance.transform.SetParent(_UnlocksContainer, false);
      tileInstance.name = _Unavailable[i].Id.Value.ToString();
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().DASEventButtonName = tileInstance.name;
      tileInstance.GetComponent<Cozmo.UI.CozmoButton>().DASEventViewController = "cozmo_unlock_panel";
      tileInstance.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = tileInstance.name + "\n(locked)";
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
      // TODO: Send message to engine to trigger unlockable action.
    }
    else if (unlockInfo.UnlockableType == UnlockableType.Game) {
      // TODO: run the game that was unlocked.
    }
  }

  private void HandleTappedAvailable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
  }

  private void HandleRequestSetUnlockResult(Anki.Cozmo.UnlockIds unlockId, bool unlocked) {
    LoadTiles();
  }
}
