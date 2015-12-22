using UnityEngine;
using System.Collections;
using Anki.UI;
using UnityEngine.UI;

public class MinesweeperGridElement : MonoBehaviour {

  public AnkiTextLabel Label;

  public GameObject Flag;

  public GameObject Hidden;

  public GameObject Mine;

  public Button HiddenButton;
  public Button FlagButton;

  private static Color[] _Colors = new Color[]{
    Color.white,
    Color.blue,
    Color.green,
    Color.red,
    new Color(0, 0, 0.5f),
    new Color(0.5f, 0.3f, 0.2f),
    Color.cyan,
    Color.black,
    Color.gray
  };

  private int _Row;
  private int _Column;
  private MinesweeperGame _Game;

  public void Initialize(MinesweeperGame game, int row, int column) {
    _Row = row;
    _Column = column;
    _Game = game;
    HiddenButton.onClick.AddListener(HandleButtonClick);
    FlagButton.onClick.AddListener(HandleButtonClick);
  }

  public void UpdateStatus(int count, MinesweeperGame.CellStatus status) {
    if (count > 0) {
      Label.text = count.ToString();
      Label.gameObject.SetActive(true);
      Label.color = _Colors[count];
    }
    else {
      Label.gameObject.SetActive(false);
    }

    Hidden.SetActive(status == MinesweeperGame.CellStatus.Hidden);
    Flag.SetActive(status == MinesweeperGame.CellStatus.Flagged);
    Mine.SetActive(count == MinesweeperGame.kMine);
  }

  private void HandleButtonClick() {
    _Game.ToggleFlag(_Row, _Column);
  }
}
