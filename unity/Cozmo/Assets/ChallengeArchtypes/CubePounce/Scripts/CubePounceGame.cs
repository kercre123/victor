using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CubePounce {
  
  public class CubePounceGame : GameBase {

    public const string kSetUp = "SetUp";
    public const string kWaitForPounce = "WaitForPounce";
    public const string kCozmoWinEarly = "CozmoWinEarly";
    public const string kCozmoWinPounce = "CozmoWinPounce";
    public const string kPlayerWin = "PlayerWin";
    // Consts for determining the exact placement and forgiveness for cube location
    // Must be consistent for animations to work
    public const float kCubePlaceDist = 60.0f;
    private int _CozmoScore;
    private int _PlayerScore;
    private int _PlayerRoundsWon;
    private int _CozmoRoundsWon;
    private int _CloseRoundCount;

    private float _MinAttemptDelay;
    private float _MaxAttemptDelay;
    private int _Rounds;

    private bool _CliffFlagTrown = false;
    private float _CurrentPounceChance;
    private float _BasePounceChance;
    private int _MaxFakeouts;
    private int _MaxScorePerRound;

    private LightCube _CurrentTarget = null;

    public bool AllRoundsCompleted {
      get {
        int losingScore = Mathf.Min(_PlayerRoundsWon, _CozmoRoundsWon);
        int winningScore = Mathf.Max(_PlayerRoundsWon, _CozmoRoundsWon);
        int roundsLeft = _Rounds - losingScore - winningScore;
        return (winningScore > losingScore + roundsLeft);
      }
    }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      CubePounceConfig config = minigameConfig as CubePounceConfig;
      _Rounds = config.Rounds;
      _MinAttemptDelay = config.MinAttemptDelay;
      _MaxAttemptDelay = config.MaxAttemptDelay;
      _BasePounceChance = config.StartingPounceChance;
      _MaxFakeouts = config.MaxFakeouts;
      _MaxScorePerRound = config.MaxScorePerRound;
      _CozmoScore = 0;
      _PlayerScore = 0;
      _PlayerRoundsWon = 0;
      _CozmoRoundsWon = 0;
      _CurrentTarget = null;
      InitializeMinigameObjects(config.NumCubesRequired());
    }

    protected void InitializeMinigameObjects(int numCubes) {

      CurrentRobot.SetBehaviorSystem(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      RobotEngineManager.Instance.OnCliffEvent += HandleCliffEvent;

      InitialCubesState initCubeState = new InitialCubesState(new HowToPlayState(new SeekState()), numCubes);
      _StateMachine.SetNextState(initCubeState);
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.OnCliffEvent -= HandleCliffEvent;
    }

    public LightCube GetCurrentTarget() {
      if (_CurrentTarget == null) {
        if (this.CubeIdsForGame.Count > 0) {
          _CurrentTarget = CurrentRobot.LightCubes[this.CubeIdsForGame[0]];
        }
      }
      return _CurrentTarget;
    }

    // Attempt the pounce
    public void AttemptPounce() {
      float PounceRoll;
      if (_MaxFakeouts <= 0) {
        PounceRoll = 0.0f;
      }
      else {
        PounceRoll = Random.Range(0.0f, 1.0f);
      }
      if (PounceRoll <= _CurrentPounceChance) {
        // Enter Animation State to attempt a pounce.
        _CliffFlagTrown = false;
        CurrentRobot.SendAnimationGroup(AnimationGroupName.kCubePounce_Pounce, HandlePounceEnd);
      }
      else {
        // If you do a fakeout instead, increase the likelyhood of a slap
        // attempt based on the max number of fakeouts.
        _CurrentPounceChance += ((1.0f - _BasePounceChance) / _MaxFakeouts);
        CurrentRobot.SendAnimationGroup(AnimationGroupName.kCubePounce_Fake, HandleFakeoutEnd);
      }
    }

    private void HandlePounceEnd(bool success) {
      // If the animation completes and the cube is beneath Cozmo,
      // Cozmo has won.
      if (_CliffFlagTrown) {
        SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinPoint);
        SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinPoint);
        OnCozmoWin();
        return;
      }
      else {
        // If the animation completes Cozmo is not on top of the Cube,
        // The player has won this round 
        SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPlayerWinPoint);
        SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoPlayerWinPoint);
        OnPlayerWin();
      }
    }

    public void HandleFakeoutEnd(bool success) {
      _StateMachine.SetNextState(new PounceState());
    }

    private void HandleCliffEvent(Anki.Cozmo.CliffEvent cliff) {
      // Ignore if it throws this without a cliff actually detected
      if (!cliff.detected) {
        return;
      }
      _CliffFlagTrown = true;
    }

    public void OnPlayerWin() {
      _PlayerScore++;
      UpdateScoreboard();
      _StateMachine.SetNextState(new AnimationState(AnimationGroupName.kCubePounce_LoseHand, HandEndAnimationDone));
    }

    public void OnCozmoWin() {
      _CozmoScore++;
      UpdateScoreboard();
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_WinHand, HandEndAnimationDone));
    }

    public void HandEndAnimationDone(bool success) {
      // Determines winner and loser at the end of Cozmo's animation, or returns
      // to seek state for the next round
      // Display the current round
      if (Mathf.Max(_CozmoScore, _PlayerScore) > _MaxScorePerRound) {
        if (_CozmoScore > _PlayerScore) {
          _CozmoRoundsWon++;
        }
        else {
          _PlayerRoundsWon++;
        }
        UpdateScoreboard();
        if (AllRoundsCompleted) {
          if (_CozmoScore > _PlayerScore) {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_WinSession, HandleLoseGameAnimationDone));
          }
          else {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_LoseSession, HandleWinGameAnimationDone));
          }
        }
        else if (_CozmoScore >= _MaxScorePerRound) {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_WinRound, RoundEndAnimationDone));
        }
        else if (_PlayerScore >= _MaxScorePerRound) {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_LoseRound, RoundEndAnimationDone));
        }
      }
      else {
        _StateMachine.SetNextState(new SeekState());
      }
    }

    public void HandleWinGameAnimationDone(bool success) {
      RaiseMiniGameWin();
    }

    public void HandleLoseGameAnimationDone(bool success) {
      RaiseMiniGameLose();
    }

    public void RoundEndAnimationDone(bool success) {
      if (Mathf.Abs(_PlayerScore - _CozmoScore) < 2) {
        _CloseRoundCount++;
      }
      _StateMachine.SetNextState(new SeekState());
    }

    public float GetAttemptDelay() {
      return Random.Range(_MinAttemptDelay, _MaxAttemptDelay);
    }

    public void ResetPounceChance() {
      _CurrentPounceChance = _BasePounceChance;
    }

    protected override int CalculateExcitementStatRewards() {
      return 1 + _CloseRoundCount * 2;
    }

    public void UpdateScoreboard() {
      int halfTotalRounds = (_Rounds + 1) / 2;
      Cozmo.MinigameWidgets.ScoreWidget cozmoScoreWidget = SharedMinigameView.CozmoScoreboard;
      cozmoScoreWidget.Score = _CozmoScore;
      cozmoScoreWidget.MaxRounds = halfTotalRounds;
      cozmoScoreWidget.RoundsWon = _CozmoRoundsWon;

      Cozmo.MinigameWidgets.ScoreWidget playerScoreWidget = SharedMinigameView.PlayerScoreboard;
      playerScoreWidget.Score = _PlayerScore;
      playerScoreWidget.MaxRounds = halfTotalRounds;
      playerScoreWidget.RoundsWon = _PlayerRoundsWon;

      if (AllRoundsCompleted) {
        // Hide Current Round at end
        SharedMinigameView.InfoTitleText = string.Empty;
      }
      else {
        // Display the current round
        SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, _CozmoRoundsWon + _PlayerRoundsWon + 1);
      }
    }
  }
}
