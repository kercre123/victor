using UnityEngine;
using System.Collections.Generic;

public class SpeedTapGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 2;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int Rounds;
  public int MaxScorePerRound;
  [Range(0.0f, 1.0f)]
  public float MinIdleInterval;
  [Range(0.0f, 1.0f)]
  public float MaxIdleInterval;
  [Range(0.0f, 1.0f)]
  public float BaseMatchChance;
  [Range(0.0f, 1.0f)]
  public float MatchChanceIncrease;
  [Range(0.0f, 1.0f)]
  public float CozmoFakeoutChance;

  [SerializeField]
  protected MusicStateWrapper _BetweenRoundMusic;

  public Anki.Cozmo.Audio.GameState.Music BetweenRoundMusic {
    get { return _BetweenRoundMusic.Music; }
  }

  public List<SpeedTapDifficultyData> DifficultySettings = new List<SpeedTapDifficultyData>();
}
