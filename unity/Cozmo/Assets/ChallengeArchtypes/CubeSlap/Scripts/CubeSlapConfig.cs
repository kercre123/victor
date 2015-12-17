using UnityEngine;
using System.Collections;

namespace CubeSlap {

  public class CubeSlapConfig : MinigameConfigBase {
    public float MinSlapDelay;
    public float MaxSlapDelay;
    public int MaxAttempts;
    public int SuccessGoal;
    public float StartingSlapChance;
    public int MaxFakeouts;
  }
}