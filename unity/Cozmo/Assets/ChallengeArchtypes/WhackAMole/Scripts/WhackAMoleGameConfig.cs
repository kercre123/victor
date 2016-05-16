using UnityEngine;
using System.Collections;

namespace WhackAMole {
  public class WhackAMoleGameConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 2;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public float MaxPanicTime;
    public float MaxConfusionTime;
    public float MaxPanicInterval;
    public float MoleTimeout;
    [Range(0f, 1f)]
    public float PanicDecayMin;
    [Range(0f, 1f)]
    public float PanicDecayMax;
  }
}
