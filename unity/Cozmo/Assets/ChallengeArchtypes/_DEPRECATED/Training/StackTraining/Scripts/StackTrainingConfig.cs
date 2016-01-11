using UnityEngine;
using System.Collections;

public class StackTrainingConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 1;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int MaxAttempts = 3;
}
