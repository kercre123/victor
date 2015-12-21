using UnityEngine;
using System.Collections;

public class CodeBreakerGameConfig : MinigameConfigBase {
  // Cozmo only supports 3 full color LEDs on his back.
  [Range(2, 3)]
  public int NumCubesInCode = 2;

  [Range(2, 6)]
  public int NumCodeColors = 6;
}
