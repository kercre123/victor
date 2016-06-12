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
  private List<CozmoUnlockableTile> _AvailableTiles;
  private List<CozmoUnlockableTile> _UnavailableTiles;

  [SerializeField]
  private RectTransform _UnlocksContainer;

  [SerializeField]
  private CozmoUnlockableTile _UnlocksTilePrefab;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _HexLabel;

  [SerializeField]
  private RequestTricksView _RequestTricksViewPrefab;
  private RequestTricksView _RequestTricksViewInstance;

  [SerializeField]
  private Sprite[] _BeginningCircuitSprites;

  [SerializeField]
  private Sprite[] _EndCircuitSprites;

  void Start() {
    _UnlockedTiles = new List<CozmoUnlockableTile>();
    _AvailableTiles = new List<CozmoUnlockableTile>();
    _UnavailableTiles = new List<CozmoUnlockableTile>();
    LoadTiles();
    RobotEngineManager.Instance.OnRequestSetUnlockResult += HandleRequestSetUnlockResult;
    UpdatePuzzlePieceCount();
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemAdded += HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemRemoved += HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemCountSet += HandleItemValueChanged;
  }

  void OnDestroy() {
    RobotEngineManager.Instance.OnRequestSetUnlockResult -= HandleRequestSetUnlockResult;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemAdded -= HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemRemoved -= HandleItemValueChanged;
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.ItemCountSet -= HandleItemValueChanged;
  }

  public void LoadTiles() {

    ClearTiles();

    string viewControllerName = "home_hub_cozmo_unlock_panel";
    int numTilesMade = 0;
    List<UnlockableInfo> unlockedUnlockData = UnlockablesManager.Instance.GetUnlocked(true);
    List<UnlockableInfo> availableUnlockData = UnlockablesManager.Instance.GetAvailableAndLockedExplicit();
    List<UnlockableInfo> unavailableUnlockData = UnlockablesManager.Instance.GetUnavailableExplicit();

    int numRows = 2;
    int numBeginningTiles = numRows;
    int endTilesStartIndex = unlockedUnlockData.Count + availableUnlockData.Count + unavailableUnlockData.Count - numRows;
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

    for (int i = 0; i < availableUnlockData.Count; ++i) {
      tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
      unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
      unlockableTile.Initialize(unlockedUnlockData[i], CozmoUnlockState.Unlockable, viewControllerName,
                                numTilesMade < numBeginningTiles, _BeginningCircuitSprites[numTilesMade % _BeginningCircuitSprites.Length],
                                numTilesMade >= endTilesStartIndex, _EndCircuitSprites[numTilesMade % _EndCircuitSprites.Length]);
      unlockableTile.OnTapped += HandleTappedAvailable;
      _AvailableTiles.Add(unlockableTile);
      numTilesMade++;
    }

    for (int i = 0; i < unavailableUnlockData.Count; ++i) {
      tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
      unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
      unlockableTile.Initialize(unlockedUnlockData[i], CozmoUnlockState.Locked, viewControllerName,
                                numTilesMade < numBeginningTiles, _BeginningCircuitSprites[numTilesMade % _BeginningCircuitSprites.Length],
                                numTilesMade >= endTilesStartIndex, _EndCircuitSprites[numTilesMade % _EndCircuitSprites.Length]);
      _UnavailableTiles.Add(unlockableTile);
      numTilesMade++;
    }
  }

  private void ClearTiles() {
    for (int i = 0; i < _UnlockedTiles.Count; ++i) {
      _UnlockedTiles[i].OnTapped -= HandleTappedUnlocked;
    }
    _UnlockedTiles.Clear();

    for (int i = 0; i < _AvailableTiles.Count; ++i) {
      _AvailableTiles[i].OnTapped -= HandleTappedAvailable;
    }
    _AvailableTiles.Clear();

    _UnavailableTiles.Clear();

    for (int i = 0; i < _UnlocksContainer.transform.childCount; ++i) {
      Destroy(_UnlocksContainer.transform.GetChild(i).gameObject);
    }
  }

  private void HandleTappedUnlocked(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped Unlocked: " + unlockInfo.Id);
    if (unlockInfo.UnlockableType == UnlockableType.Action) {
      _RequestTricksViewInstance = UIManager.OpenView<RequestTricksView>(_RequestTricksViewPrefab);
      _RequestTricksViewInstance.Initialize(unlockInfo);
    }
    else if (unlockInfo.UnlockableType == UnlockableType.Game) {
      // TODO: run the game that was unlocked.
    }
  }

  private void HandleTappedAvailable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    // TODO: This const should be replaced by the actual hex puzzle system when that's done.
    string hexPieceId = "TestHexItem0";
    int unlockCost = 2;

    Cozmo.Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(hexPieceId, unlockCost)) {
      playerInventory.RemoveItemAmount(hexPieceId, unlockCost);
      UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
    }
  }

  private void HandleRequestSetUnlockResult(Anki.Cozmo.UnlockId unlockId, bool unlocked) {
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
