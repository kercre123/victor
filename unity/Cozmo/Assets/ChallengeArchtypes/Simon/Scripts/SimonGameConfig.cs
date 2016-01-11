using UnityEngine;
using System.Collections;

public class SimonGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 1;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int MinSequenceLength = 3;
  public int MaxSequenceLength = 10;
  public int MaxAttempts = 3;
}
