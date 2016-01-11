using UnityEngine;
using System.Collections;

public class SelfieGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 1;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int CountdownTimer = 10;
}
