using UnityEngine;
using System.Collections;

public class MotionDetectionConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 1;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public float TimeAllowedBetweenWaves = 1.0f;

  public float TotalWaveTime = 5.0f;
}
