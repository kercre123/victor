using UnityEngine;
using System.Collections.Generic;

public class SpeedTapGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 2;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  [Tooltip("The amount of time in seconds that remaining 'Cubes On' time is cut to if a tap message is received")]
  public float TapResolutionDelay;

  [Tooltip("Total number of rounds in Speed Tap. Best of X rounds.")]
  public int Rounds;

  [Tooltip("Points required to win a round. First to X points.")]
  public int MaxScorePerRound;

  [Range(0.0f, 1.0f)]
  public float MinIdleInterval_percent;
  [Range(0.0f, 1.0f)]
  public float MaxIdleInterval_percent;
  [Range(0.0f, 1.0f)]
  public float BaseMatchChance;
  [Range(0.0f, 1.0f)]
  public float MatchChanceIncrease;
  [Range(0.0f, 1.0f)]
  public float CozmoFakeoutChance;

  public Color CozmoTint;
  public Color Player1Tint;
  public Color Player2Tint;
  // Cozmo's tint is only used for cubes. Doesn't need on scoreboards.
  public Color Player1TintDim;
  public Color Player2TintDim;

  public float MPTimeBetweenRoundsSec = 5.0f;
  public float MPTimeSetupHoldSec = 5.0f;

  [SerializeField]
  protected MusicStateWrapper _BetweenRoundMusic;

  public Anki.Cozmo.Audio.GameState.Music BetweenRoundMusic {
    get { return _BetweenRoundMusic.Music; }
  }

  public List<SpeedTapDifficultyData> DifficultySettings = new List<SpeedTapDifficultyData>();
}
