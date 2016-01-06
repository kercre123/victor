using UnityEngine;
using System.Collections;

namespace WhackAMole {
  public class WhackAMoleGameConfig : MinigameConfigBase {
    public float MaxPanicTime;
    public float MaxConfusionTime;
    public float MaxPanicInterval;
    [Range(0f,1f)]
    public float PanicDecayMin;
    [Range(0f,1f)]
    public float PanicDecayMax;
  }
}
