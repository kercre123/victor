using UnityEngine;
using System.Collections;

namespace CubePounce {

  public class CubePounceConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 1;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public float MinSlapDelay;
    public float MaxSlapDelay;
    public int Rounds;
    public int MaxScorePerRound;
    [Range(0f, 1f)]
    public float StartingSlapChance;
    public int MaxFakeouts;
  }
}