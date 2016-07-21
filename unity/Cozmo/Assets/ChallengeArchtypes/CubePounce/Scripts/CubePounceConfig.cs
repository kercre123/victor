using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {

  public class CubePounceConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 1;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public float MinAttemptDelay;
    public float MaxAttemptDelay;
    public int Rounds;
    public int MaxScorePerRound;
    [Range(0f, 1f)]
    public float StartingPounceChance;
    public int MaxFakeouts;


    [SerializeField]
    protected MusicStateWrapper _BetweenRoundMusic;

    public Anki.Cozmo.Audio.GameState.Music BetweenRoundMusic {
      get { return _BetweenRoundMusic.Music; }
    }

  }
}