using UnityEngine;
using System.Collections;

public class SimonGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return NumCubesInPattern;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int MinSequenceLength = 3;
  public int MaxSequenceLength = 10;
  public int MaxAttempts = 3;

  [Range(2, 5)]
  public int NumCubesInPattern = 2;

  [Range(0f, 1f)]
  public float CozmoGuessCubeCorrectPercentage = 0.9f;
}
