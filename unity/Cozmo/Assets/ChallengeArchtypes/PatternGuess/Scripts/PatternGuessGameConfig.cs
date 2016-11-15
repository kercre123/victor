using UnityEngine;
using System.Collections;

public class PatternGuessGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return NumCubesInCode;
  }

  public override int NumPlayersRequired() {
    return 1;
  }
  // Cozmo only supports 3 full color LEDs on his back.
  [Range(2, 3)]
  public int NumCubesInCode = 2;

  [Range(2, 6)]
  public int NumCodeColors = 6;

  [Range(1, 100)]
  public int NumGuesses = 10;
}
