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
  public float PlayNewHandIntervalMs;
  public float PlayNewHandRevealMs;
  public float MinIdleIntervalMs;
  public float MaxIdleIntervalMs;
  public float BaseMatchChance;
  public float MatchChanceIncrease;

  [SerializeField]
  protected MusicStateWrapper _BetweenRoundMusic;

  public Anki.Cozmo.Audio.GameState.Music BetweenRoundMusic {
    get { return _BetweenRoundMusic.Music; }
  }

  // These two lists must be the same length
  public List<DifficultySelectOptionData> DifficultyOptions = new List<DifficultySelectOptionData>();
  public List<SpeedTapDifficultyData> DifficultySettings = new List<SpeedTapDifficultyData>();
}
