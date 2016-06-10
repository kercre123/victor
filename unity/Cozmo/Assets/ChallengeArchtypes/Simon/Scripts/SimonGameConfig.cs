using UnityEngine;
using System.Collections;

public class SimonGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 3;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int MinSequenceLength = 1;
  public int MaxSequenceLength = 20;

  public float TimeBetweenBeats = 0.3f;
  public float TimeWaitFirstBeat = 1.0f;

  public AnimationCurve CozmoGuessCubeCorrectPercentage = new AnimationCurve(new Keyframe(0, 1, 0, -0.06f), new Keyframe(5, 0.7f, -0.06f, 0));

  public MusicStateWrapper BetweenRoundsMusic;

  public Color CubeTooFarColor = new Color(1.0f, 1.0f, 0.0f);
  public Color CubeTooCloseColor = new Color(1.0f, 0.0f, 0.0f);
}
