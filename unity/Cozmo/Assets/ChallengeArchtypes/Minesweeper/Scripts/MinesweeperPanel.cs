using UnityEngine;
using System.Collections;

public class MinesweeperPanel : MonoBehaviour {

  public RectTransform Grid;

  public GameObject GridRowPrefab;

  public GameObject GridEntryPrefab;

  private MinesweeperGridElement[,] _GridElements;

  private MinesweeperGame _Game;

  public void Initialize(MinesweeperGame game) {
    _Game = game;
    _Game.OnGridCellUpdated += HandleGridCellUpdated;

    _GridElements = new MinesweeperGridElement[game.Rows, game.Columns];
    for (int i = 0; i < _Game.Rows; i++) {

      var row = GameObject.Instantiate<GameObject>(GridRowPrefab);

      var rowTransform = row.transform;
      rowTransform.SetParent(Grid, false);

      for (int j = 0; j < _Game.Columns; j++) {

        var entry = GameObject.Instantiate<GameObject>(GridEntryPrefab);

        var minesweeperElement = entry.GetComponent<MinesweeperGridElement>();

        _GridElements[i, j] = minesweeperElement;
        minesweeperElement.Initialize(_Game, i, j);

        minesweeperElement.transform.SetParent(rowTransform, false);
      }
    }
  }

  void HandleGridCellUpdated (int row, int col, int count, MinesweeperGame.CellStatus status)
  {
    _GridElements[row, col].UpdateStatus(count, status);
  }
}
