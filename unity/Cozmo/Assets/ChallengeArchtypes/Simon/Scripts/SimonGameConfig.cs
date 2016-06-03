using UnityEngine;
using System.Collections;

public class SimonGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return NumCubesInPattern;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int MinSequenceLength = 1;
  public int MaxSequenceLength = 20;

  [Range(2, 5)]
  public int NumCubesInPattern = 2;

  public AnimationCurve CozmoGuessCubeCorrectPercentage = new AnimationCurve(new Keyframe(0, 1, 0, -0.06f), new Keyframe(5, 0.7f, -0.06f, 0));

  public MusicStateWrapper BetweenRoundsMusic;
}
