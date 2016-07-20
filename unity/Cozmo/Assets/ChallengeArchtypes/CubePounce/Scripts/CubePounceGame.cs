using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo.Minigame.CubePounce {
  // TODO : SCORE RELATED LOGIC HAS BEEN MOVED TO THE SCORING REGION OF GAMEBASE, private fields for score/rounds are obsolete
  public class CubePounceGame : GameBase {

    public const string kSetUp = "SetUp";
    public const string kWaitForPounce = "WaitForPounce";
    public const string kCozmoWinEarly = "CozmoWinEarly";
    public const string kCozmoWinPounce = "CozmoWinPounce";
    public const string kPlayerWin = "PlayerWin";
    // Consts for determining the exact placement and forgiveness for cube location
    // Must be consistent for animations to work
    private const float _kWheelCenterToFacePlane_mm = 13.0f;
    // Current animations expect to start from 55 mm back
    public const float kCubePlaceDist_mm = 55.0f + _kWheelCenterToFacePlane_mm;
    public const float kPositionDiffTolerance_mm = 8.0f;
    public const float kAngleTolerance_deg = 15.0f;
    public const float kPouncePitchDiffSuccess_deg = 5.0f;
    public const float kPounceLiftMeasureDelay_s = 0.15f;

    public const float kCubeLightFlashInterval_m = 0.1f;

    private int _CloseRoundCount;

    private float _MinAttemptDelay_s;
    private float _MaxAttemptDelay_s;

    private float _CurrentPounceChance;
    private float _BasePounceChance;
    private int _MaxFakeouts;
    private int _NumCubesRequired;

    private MusicStateWrapper _BetweenRoundsMusic;

    private LightCube _CurrentTarget = null;

    public bool AllRoundsCompleted {
      get {
        int winningScore = Mathf.Max(PlayerRoundsWon, CozmoRoundsWon);
        return (2 * winningScore > TotalRounds);
      }
    }

    public int NumCubesRequired {
      get {
        return _NumCubesRequired;
      }
      private set {
        _NumCubesRequired = value;
      }
    }

    public MusicStateWrapper BetweenRoundsMusic {
      get {
        return _BetweenRoundsMusic;
      }
    }

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {
      CubePounceConfig config = minigameConfig as CubePounceConfig;
      TotalRounds = config.Rounds;
      _MinAttemptDelay_s = config.MinAttemptDelay;
      _MaxAttemptDelay_s = config.MaxAttemptDelay;
      _BasePounceChance = config.StartingPounceChance;
      _MaxFakeouts = config.MaxFakeouts;
      NumCubesRequired = config.NumCubesRequired();
      MaxScorePerRound = config.MaxScorePerRound;
      CozmoScore = 0;
      PlayerScore = 0;
      PlayerRoundsWon = 0;
      CozmoRoundsWon = 0;
      _CurrentTarget = null;
      _BetweenRoundsMusic = config.BetweenRoundMusic;
      InitializeMinigameObjects(NumCubesRequired);
    }

    protected void InitializeMinigameObjects(int numCubes) {

      CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      _StateMachine.SetNextState(new InitialCubesState(new CubePounceStateInitGame(), numCubes));
    }

    protected override void CleanUpOnDestroy() {
    }

    public LightCube GetCubeTarget() {
      if (_CurrentTarget == null) {
        if (this.CubeIdsForGame.Count > 0) {
          _CurrentTarget = CurrentRobot.LightCubes[this.CubeIdsForGame[0]];
        }
      }
      return _CurrentTarget;
    }

    public bool ShouldAttemptPounce() {
      DAS.Debug("CubePounceGame.ShouldAttemptPounce", "Pounce chance: " + _CurrentPounceChance);
      if (Random.Range(0.0f, 1.0f) <= _CurrentPounceChance) {
        return true;
      }

      return false;
    }

    public void IncreasePounceChance() {
      if (_MaxFakeouts <= 0) {
        return;
      }

      // Increase the likelyhood of a pounce based on the max number of fakeouts.
      _CurrentPounceChance += ((1.0f - _BasePounceChance) / _MaxFakeouts);
    }

    // Returns whether we just finished a round
    public bool CheckAndUpdateRoundScore() {
      // If we haven't yet hit our max score nothing to see here
      if (Mathf.Max(CozmoScore, PlayerScore) < MaxScorePerRound) {
        return false;
      }

      if (CozmoScore > PlayerScore) {
        CozmoRoundsWon++;
      }
      else {
        PlayerRoundsWon++;
      }
      return true;
    }

    public float GetAttemptDelay() {
      return Random.Range(_MinAttemptDelay_s, _MaxAttemptDelay_s);
    }

    public void ResetPounceChance() {
      DAS.Debug("CubePounceGame.ResetPounceChance", "");
      _CurrentPounceChance = _BasePounceChance;
    }

    public void UpdateScoreboard() {
      int halfTotalRounds = (TotalRounds + 1) / 2;
      Cozmo.MinigameWidgets.ScoreWidget cozmoScoreWidget = SharedMinigameView.CozmoScoreboard;
      cozmoScoreWidget.Score = CozmoScore;
      cozmoScoreWidget.MaxRounds = halfTotalRounds;
      cozmoScoreWidget.RoundsWon = CozmoRoundsWon;

      Cozmo.MinigameWidgets.ScoreWidget playerScoreWidget = SharedMinigameView.PlayerScoreboard;
      playerScoreWidget.Score = PlayerScore;
      playerScoreWidget.MaxRounds = halfTotalRounds;
      playerScoreWidget.RoundsWon = PlayerRoundsWon;

      if (AllRoundsCompleted) {
        // Hide Current Round at end
        SharedMinigameView.InfoTitleText = string.Empty;
      }
      else {
        // Display the current round
        SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, CozmoRoundsWon + PlayerRoundsWon + 1);
      }
    }

    protected override void SendCustomEndGameDasEvents() {
      Dictionary<string, string> quitGameScoreKeyValues = new Dictionary<string, string>();
      Dictionary<string, string> quitGameRoundsWonKeyValues = new Dictionary<string, string>();

      quitGameScoreKeyValues.Add("CozmoScore", CozmoScore.ToString());
      quitGameRoundsWonKeyValues.Add("CozmoRoundsWon", CozmoRoundsWon.ToString());

      DAS.Event(DASConstants.Game.kQuitGameScore, PlayerScore.ToString(), null, quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, PlayerRoundsWon.ToString(), null, quitGameRoundsWonKeyValues);

      GetCubeTarget().SetLEDsOff();
    }
  }
}
