using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public struct ForwardBackwardSetting {
  public bool Forward;
  public float Distance;
}

public class ForwardBackwardConfig : MinigameConfigBase {

  public List<ForwardBackwardSetting> Settings;
}
