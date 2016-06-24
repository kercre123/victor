using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

public class CozmoUnlocksPanel : MonoBehaviour {

  public enum CozmoUnlockState {
    Unlocked,
    Unlockable,
    Locked
  }

  public enum CozmoUnlockPosition {
    Beginning,
    Middle,
    End
  }

  private List<CozmoUnlockableTile> _UnlockedTiles;
  private List<CozmoUnlockableTile> _UnlockableTiles;
  private List<CozmoUnlockableTile> _LockedTiles;

  [SerializeField]
  private RectTransform _UnlocksContainer;

  [SerializeField]
  private CozmoUnlockableTile _UnlocksTilePrefab;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _HexLabel;

  [SerializeField]
  private CoreUpgradeDetailsDialog _CoreUpgradeDetailsViewPrefab;
  private CoreUpgradeDetailsDialog _CoreUpgradeDetailsViewInstance;

  [SerializeField]
  private Sprite[] _BeginningCircuitSprites;

  [SerializeField]
  private Sprite[] _EndCircuitSprites;

  void Start() {
    _UnlockedTiles = new List<CozmoUnlockableTile>();
    _UnlockableTiles = new List<CozmoUnlockableTile>();
    _LockedTiles = new List<CozmoUnlockableTile>();
    LoadTiles();
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleRequestSetUnlockResult);
    UpdatePuzzlePieceCount();
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemAdded += HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemRemoved += HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemCountSet += HandleItemValueChanged;
  }

  void OnDestroy() {
    if (_CoreUpgradeDetailsViewInstance != null) {
      _CoreUpgradeDetailsViewInstance.CloseView();
    }

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleRequestSetUnlockResult);
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemAdded -= HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemRemoved -= HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemCountSet -= HandleItemValueChanged;
  }

  public void LoadTiles() {

    ClearTiles();

    string viewControllerName = "home_hub_cozmo_unlock_panel";
    int numTilesMade = 0;
    List<UnlockableInfo> unlockedUnlockData = UnlockablesManager.Instance.GetUnlocked(true);
    List<UnlockableInfo> unlockableUnlockData = UnlockablesManager.Instance.GetAvailableAndLockedExplicit();
    List<UnlockableInfo> lockedUnlockData = UnlockablesManager.Instance.GetUnavailableExplicit();

    int numRows = 2;
    int numBeginningTiles = numRows;
    int endTilesStartIndex = unlockedUnlockData.Count + unlockableUnlockData.Count + lockedUnlockData.Count - numRows;
    GameObject tileInstance;
    CozmoUnlockableTile unlockableTile;
    for (int i = 0; i < unlockedUnlockData.Count; ++i) {
      tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
      unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
      unlockableTile.Initialize(unlockedUnlockData[i], CozmoUnlockState.Unlocked, viewControllerName,
        numTilesMade < numBeginningTiles, _BeginningCircuitSprites[numTilesMade % _BeginningCircuitSprites.Length],
        numTilesMade >= endTilesStartIndex, _EndCircuitSprites[numTilesMade % _EndCircuitSprites.Length]);
      unlockableTile.OnTapped += HandleTappedUnlocked;
      _UnlockedTiles.Add(unlockableTile);
      numTilesMade++;
    }

    for (int i = 0; i < unlockableUnlockData.Count; ++i) {
      tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
      unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
      unlockableTile.Initialize(unlockableUnlockData[i], CozmoUnlockState.Unlockable, viewControllerName,
        numTilesMade < numBeginningTiles, _BeginningCircuitSprites[numTilesMade % _BeginningCircuitSprites.Length],
        numTilesMade >= endTilesStartIndex, _EndCircuitSprites[numTilesMade % _EndCircuitSprites.Length]);
      unlockableTile.OnTapped += HandleTappedUnlockable;
      _UnlockableTiles.Add(unlockableTile);
      numTilesMade++;
    }

    for (int i = 0; i < lockedUnlockData.Count; ++i) {
      tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
      unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
      unlockableTile.Initialize(lockedUnlockData[i], CozmoUnlockState.Locked, viewControllerName,
        numTilesMade < numBeginningTiles, _BeginningCircuitSprites[numTilesMade % _BeginningCircuitSprites.Length],
        numTilesMade >= endTilesStartIndex, _EndCircuitSprites[numTilesMade % _EndCircuitSprites.Length]);
      _LockedTiles.Add(unlockableTile);
      numTilesMade++;
    }
  }

  private void ClearTiles() {
    for (int i = 0; i < _UnlockedTiles.Count; ++i) {
      _UnlockedTiles[i].OnTapped -= HandleTappedUnlocked;
    }
    _UnlockedTiles.Clear();

    for (int i = 0; i < _UnlockableTiles.Count; ++i) {
      _UnlockableTiles[i].OnTapped -= HandleTappedUnlockable;
    }
    _UnlockableTiles.Clear();

    _LockedTiles.Clear();

    for (int i = 0; i < _UnlocksContainer.transform.childCount; ++i) {
      Destroy(_UnlocksContainer.transform.GetChild(i).gameObject);
    }
  }

  private void HandleTappedUnlocked(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped Unlocked: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null) {
      _CoreUpgradeDetailsViewInstance = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      _CoreUpgradeDetailsViewInstance.Initialize(unlockInfo, CozmoUnlockState.Unlocked, null);
    }
  }

  private void HandleTappedUnlockable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null) {
      _CoreUpgradeDetailsViewInstance = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      _CoreUpgradeDetailsViewInstance.Initialize(unlockInfo, CozmoUnlockState.Unlockable, HandleUnlockableUpgradeUnlocked);
    }
  }

  private void HandleUnlockableUpgradeUnlocked(UnlockableInfo unlockInfo) {
    UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
  }

  private void HandleRequestSetUnlockResult(object message) {
    LoadTiles();
  }

  private void HandleItemValueChanged(string itemId, int delta, int count) {
    if (Cozmo.HexItemList.IsPuzzlePiece(itemId)) {
      UpdatePuzzlePieceCount();
    }
  }

  private void UpdatePuzzlePieceCount() {
    IEnumerable<string> puzzlePieceIds = Cozmo.HexItemList.GetPuzzlePieceIds();
    int totalNumberHexes = 0;
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    foreach (string puzzlePieceId in puzzlePieceIds) {
      totalNumberHexes += playerInventory.GetItemAmount(puzzlePieceId);
    }
    _HexLabel.FormattingArgs = new object[] { totalNumberHexes };
  }
}
