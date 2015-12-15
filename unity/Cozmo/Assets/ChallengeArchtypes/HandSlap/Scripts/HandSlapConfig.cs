using UnityEngine;
using System.Collections;

namespace HandSlap {

  public class HandSlapConfig : MinigameConfigBase {
    public float MinSlapDelay;
    public float MaxSlapDelay;
    public int MaxAttempts;
    public int SuccessGoal;
  }
}