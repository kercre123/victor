using UnityEngine;
using System.Collections;

public class MinesweeperConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 1;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int Rows;
  public int Columns;
  public int Mines;
}
