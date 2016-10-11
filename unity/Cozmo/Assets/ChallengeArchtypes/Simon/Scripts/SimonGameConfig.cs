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

  public float MinDistBetweenCubesMM = 60.0f;
  public float RotateSecScan = 2.0f;

  public float ScanTimeoutSec = 30.0f;

  public int MaxLivesCozmo = 3;
  public int MaxLivesHuman = 3;

  public int LongSequenceReactMin = 5;

  public AnimationCurve[] CozmoGuessCubeCorrectPercentagePerSkill;

}
