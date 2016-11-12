using UnityEngine;
using System.Collections;

public class MemoryMatchGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 3;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int MinSequenceLength = 1;
  public int MaxSequenceLength = 20;

  public float TimeWaitFirstBeat = 1.0f;

  public float MinDistBetweenCubesMM = 60.0f;
  public float RotateSecScan = 2.0f;

  public float ScanTimeoutSec = 30.0f;
  public float CountDownTimeSec = 2.25f;
  public float HoldLightsAfterCountDownTimeSec = 1.0f;
  public float IdleTimeoutSec = 60.0f;

  public int MaxLivesCozmo = 3;
  public int MaxLivesHuman = 3;

  public int MinRoundBeforeLongReact = 5;

  public AnimationCurve TimeBetweenBeat_Sec;

  public AnimationCurve[] CozmoGuessCubeCorrectPercentagePerSkill;

}
