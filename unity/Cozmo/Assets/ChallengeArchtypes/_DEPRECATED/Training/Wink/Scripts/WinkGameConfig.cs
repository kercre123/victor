using UnityEngine;
using System.Collections;

namespace Wink {
  public class WinkGameConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 1;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public float TimeLimit;
    public int MaxAttempts;
    public int WaveSuccessGoal;
  }
}
