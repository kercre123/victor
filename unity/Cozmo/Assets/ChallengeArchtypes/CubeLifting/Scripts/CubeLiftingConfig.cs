using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public struct CubeLiftingSetting {
  public bool Up;
  public bool RaiseLift;
}

public class CubeLiftingConfig : MinigameConfigBase {

  public int MaxAttempts = 3;

  public List<CubeLiftingSetting> Settings;
}
