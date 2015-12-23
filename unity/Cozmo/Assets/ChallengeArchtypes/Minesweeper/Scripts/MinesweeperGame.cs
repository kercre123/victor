using UnityEngine;
using System.Collections;
using System;
using Minesweeper;

public class MinesweeperGame : GameBase {

  [SerializeField]
  private GameObject _MinesweeperPanelPrefab;

  private MinesweeperConfig _Config;

  public const int kMine = -1;
  public const float kCellWidth = 45f;

  private bool _FirstGuess;

  private int[,] _Grid;

  public int[,] Grid { get { return _Grid; } }

  public enum CellStatus {
    Hidden,
    Visible,
    Flagged
  }

  private CellStatus[,] _GridStatus;

  public CellStatus[,] GridStatus {
    get { return _GridStatus; }
  }

  public int Rows {
    get { return _Config.Rows; } 
  }

  public int Columns {
    get { return _Config.Columns; }
  }

  public event Action<int, int, int, CellStatus> OnGridCellUpdated;

  private MinesweeperPanel _Panel;

  protected override void Initialize(MinigameConfigBase minigameConfigData) {
    _FirstGuess = true;
    _Config = (MinesweeperConfig)minigameConfigData;
    _Grid = new int[_Config.Rows, _Config.Columns];
    _GridStatus = new CellStatus[_Config.Rows, _Config.Columns];

    ResetGrid();
    var initialCubeState = new InitialCubesState();
    initialCubeState.InitialCubeRequirements(new SetupGridState(), 2, false);
    _StateMachine.SetNextState(initialCubeState);
  }

  public void SetupPanel() {
    var panelObject = UIManager.CreateUIElement(_MinesweeperPanelPrefab);

    panelObject.transform.SetParent(SharedMinigameViewInstance.transform, false);

    _Panel = panelObject.GetComponent<MinesweeperPanel>();

    _Panel.Initialize(this);
  }

  private void ResetGrid() {
    for (int i = 0; i < _Config.Rows; i++) {
      for (int j = 0; j < _Config.Columns; j++) {
        _Grid[i, j] = 0;
      }
    }

    for (int i = 0; i < _Config.Mines; i++) {
      int row, col;
      do {
        row = UnityEngine.Random.Range(0, _Config.Rows);
        col = UnityEngine.Random.Range(0, _Config.Columns);

      } while(_Grid[row, col] == kMine);
      _Grid[row, col] = kMine;
      for (int j = Mathf.Max(row - 1, 0); j < Mathf.Min(row + 2, _Config.Rows); j++) {
        for (int k = Mathf.Max(col - 1, 0); k < Mathf.Min(col + 2, _Config.Columns); k++) {
          if (_Grid[j, k] != kMine) {
            _Grid[j, k]++;
          }
        }
      }
    }
  }

  public bool TryRevealSpace(int row, int col) {
    if (row < 0 || col < 0 || row >= Rows || col >= Columns) {
      return false;
    }
    if (_GridStatus[row, col] == CellStatus.Visible) {
      return _Grid[row, col] != kMine;
    }

    bool result = true;
    if (_Grid[row, col] == kMine) {
      // we never want to trigger a mine on the very first guess
      // because that sucks. Reset the grid if it happens
      if (_FirstGuess) {
        ResetGrid();
        return TryRevealSpace(row, col);
      }
      result = false;
    }
    _FirstGuess = false;
    _GridStatus[row, col] = CellStatus.Visible;

    if (OnGridCellUpdated != null) {
      OnGridCellUpdated(row, col, _Grid[row, col], _GridStatus[row, col]);
    }

    // if you hit a space that has no mines around it, expand
    // all spaces adjacent and diagonal to it.
    if (_Grid[row, col] == 0) {
      TryRevealSpace(row + 1, col);
      TryRevealSpace(row - 1, col);
      TryRevealSpace(row, col + 1);
      TryRevealSpace(row, col - 1);
      TryRevealSpace(row + 1, col + 1);
      TryRevealSpace(row - 1, col + 1);
      TryRevealSpace(row + 1, col - 1);
      TryRevealSpace(row - 1, col - 1);
    }

    return result;
  }

  public void ToggleFlag(int row, int col) {
    if (_GridStatus[row, col] == CellStatus.Visible) {
      return;
    }
    _GridStatus[row, col] = 
        _GridStatus[row, col] == CellStatus.Hidden ? 
           CellStatus.Flagged : 
           CellStatus.Hidden;
    if (OnGridCellUpdated != null) {
      OnGridCellUpdated(row, col, _Grid[row, col], _GridStatus[row, col]);
    }
  }

  public bool IsComplete() {    
    for (int i = 0; i < _Config.Rows; i++) {
      for (int j = 0; j < _Config.Columns; j++) {
        if (_Grid[i, j] >= 0 && _GridStatus[i, j] != CellStatus.Visible) {
          return false;
        }
      }
    }
    return true;
  }

  protected override void CleanUpOnDestroy() {
    if (_Panel) {
      GameObject.Destroy(_Panel);
    }
  }
}
