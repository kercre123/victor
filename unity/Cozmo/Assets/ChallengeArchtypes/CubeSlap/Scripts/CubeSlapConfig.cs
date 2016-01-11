using UnityEngine;
using System.Collections;

namespace CubeSlap {

  public class CubeSlapConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 1;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public float MinSlapDelay;
    public float MaxSlapDelay;
    public int MaxAttempts;
    public int SuccessGoal;
    [Range(0f, 1f)]
    public float StartingSlapChance;
    public int MaxFakeouts;
  }
}