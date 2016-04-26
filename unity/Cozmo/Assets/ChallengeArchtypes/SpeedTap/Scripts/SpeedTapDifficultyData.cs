using UnityEngine;
using System.Collections.Generic;

/// <summary>
/// Speed tap data for individual rounds.
/// </summary>
[System.Serializable]
public class SpeedTapRoundData {
  [SerializeField]
  protected MusicStateWrapper _MidRoundMusic;

  public Anki.Cozmo.Audio.GameState.Music MidRoundMusic {
    get { return _MidRoundMusic.Music; }
  }

  public float TimeBetweenHands;
  public float TimeHandDisplayed;
}

/// <summary>
/// Speed tap unique difficulty settings.
/// </summary>
[System.Serializable]
public class SpeedTapDifficultyData {
  public int DifficultyID;
  public Color[] Colors;
  public List<SpeedTapRoundData> SpeedTapRoundSettings;
}
