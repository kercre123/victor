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

    public float MinAttemptDelay_s;
    public float MaxAttemptDelay_s;
    public int Rounds;
    public int MaxScorePerRound;

    [Range(0f, 1f)]
    public float BasePounceChance;
    // Amount to increase chance of pounce after each fakeout
    [Range(0f,1f)]
    public float PounceChanceIncrement; // = 0.15f

    [SerializeField]
    protected MusicStateWrapper _BetweenRoundMusic;
    public Anki.Cozmo.Audio.GameState.Music BetweenRoundMusic { get { return _BetweenRoundMusic.Music; } }

    // Distance that defines how close a block can be to be pounced on
    [Range(0f,100f)]
    public float CubeDistanceBetween_mm; // = 55f;

    [Range(0f,100f)]
    public float CubeDistanceTooClose_mm; // = 30f;

    // The distance away from the cube pounce activation line that causes cozmo to stop being able to pounce
    [Range(0f,300f)]
    public float CubeDistanceGreyZone_mm; // = 50f;

    // Number of degrees difference in Cozmos pitch to be considered a success when pouncing
    [Range(0f,90f)]
    public float PouncePitchDiffSuccess_deg; // = 5.0f;

    // Frequency with which to flash the cube lights
    [Range(0f,1f)]
    public float CubeLightFlashInterval_s; // = 0.1f;

    // Time that can elapse without having seen the cube before it's officially gone
    [Range(0f,10f)]
    public float CubeVisibleBufferTime_s; // = 1f

    // Amount of time that cube can be moved during fakeout without granting a point to Cozmo
    [Range(0f,3f)]
    public float FakeoutCubeMoveTime_s; // = 0.5f

    // Number of degrees tolerance for Cozmo to be facing toward the cube
    [Range(0f,90f)]
    public float CubeFacingAngleTolerance_deg; // = 5.0f;

    [Range(0.000001f,10f)]
    public float TurnSpeed_rps; // = 100f;

    [Range(0.000001f,100f)]
    public float TurnAcceleration_rps2; // = 100f;

    public bool FakeoutMovePenaltyEnabled; // = true;
  }
}