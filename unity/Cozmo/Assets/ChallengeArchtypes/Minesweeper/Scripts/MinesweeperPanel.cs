using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Linq;

public class MinesweeperPanel : MonoBehaviour {

  public RectTransform Grid;

  public RotateHandle RotateHandle;

  public GameObject GridRowPrefab;

  public GameObject GridEntryPrefab;

  public GameObject GridCornerPrefab;

  private MinesweeperGridElement[,] _GridElements;

  private MinesweeperGame _Game;

  private int _LastOrientation;

  private Color[] _CornerColors = new Color[4] {
    Color.magenta,
    Color.green,
    Color.cyan,
    Color.yellow
  };

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

    var cubeIds = _Game.CurrentRobot.LightCubes.Keys.ToArray();

    for (int i = 0; i < Mathf.Min(cubeIds.Length, _CornerColors.Length); i++) {
      _Game.CurrentRobot.LightCubes[cubeIds[i]].SetLEDs(_CornerColors[i]);

      var cornerObject = GameObject.Instantiate<GameObject>(GridCornerPrefab);
      var corner = cornerObject.GetComponent<MinesweeperCornerCube>();
      corner.transform.SetParent(Grid, false);
      corner.SetCorner((MinesweeperCornerCube.Corner)i, _CornerColors[i]);
    }

    RotateHandle.OnRotationUpdated += HandleRotation;
  }

  void HandleRotation (float angle)
  {
    Grid.localRotation = Quaternion.Euler(0, 0, angle);

    int orientation = Mathf.RoundToInt(angle / 90f) * 90;
    if (_LastOrientation != orientation) {
      for (int i = 0; i < _Game.Rows; i++) {
        for (int j = 0; j < _Game.Columns; j++) {
          _GridElements[i, j].transform.localRotation = Quaternion.Euler(0, 0, -orientation);
        }
      }
      _LastOrientation = orientation;
    }
  }

  void HandleGridCellUpdated (int row, int col, int count, MinesweeperGame.CellStatus status)
  {
    _GridElements[row, col].UpdateStatus(count, status);
  }


}
