using UnityEngine;
using System.Collections;

public class CodeBreakerGameConfig : MinigameConfigBase {
  [Range(2, 4)]
  public int NumCubesInCode = 2;

  [Range(2, 6)]
  public int NumCodeColors = 6;
}
