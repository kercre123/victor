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
    public const float kCubePlaceDist = 80.0f;
    private int _CozmoScore;
    private int _PlayerScore;
    private int _PlayerRoundsWon;
    private int _CozmoRoundsWon;
    private int _CloseRoundCount;

    private float _MinSlapDelay;
    private float _MaxSlapDelay;
    private int _Rounds;

    private bool _CliffFlagTrown = false;
    private float _CurrentSlapChance;
    private float _BaseSlapChance;
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
      _MinSlapDelay = config.MinSlapDelay;
      _MaxSlapDelay = config.MaxSlapDelay;
      _BaseSlapChance = config.StartingSlapChance;
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
    public void AttemptSlap() {
      float SlapRoll;
      if (_MaxFakeouts <= 0) {
        SlapRoll = 0.0f;
      }
      else {
        SlapRoll = Random.Range(0.0f, 1.0f);
      }
      if (SlapRoll <= _CurrentSlapChance) {
        // Enter Animation State to attempt a pounce.
        // Set Callback for HandleEndSlapAttempt
        _CliffFlagTrown = false;
        CurrentRobot.SendAnimationGroup(AnimationGroupName.kSpeedTap_Tap, HandleEndSlapAttempt);
      }
      else {
        // If you do a fakeout instead, increase the likelyhood of a slap
        // attempt based on the max number of fakeouts.
        _CurrentSlapChance += ((1.0f - _BaseSlapChance) / _MaxFakeouts);
        _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_Fake, HandleFakeoutEnd));
      }
    }

    private void HandleEndSlapAttempt(bool success) {
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
      _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorFail, HandEndAnimationDone));
    }

    public void OnCozmoWin() {
      _CozmoScore++;
      UpdateScoreboard();
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kWin, HandEndAnimationDone));
    }

    public void HandEndAnimationDone(bool success) {
      // Determines winner and loser at the end of Cozmo's animation, or returns
      // to seek state for the next round
      // Display the current round
      UpdateRoundsUI();
      if (Mathf.Max(_CozmoScore, _PlayerScore) > _MaxScorePerRound) {
        if (_CozmoScore > _PlayerScore) {
          _CozmoRoundsWon++;
        }
        else {
          _PlayerRoundsWon++;
        }
        if (AllRoundsCompleted) {
          if (_CozmoScore > _PlayerScore) {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_WinSession, HandleLoseGameAnimationDone));
          }
          else {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_LoseSession, HandleWinGameAnimationDone));
          }
        }
        else if (_CozmoScore >= _MaxScorePerRound) {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_WinRound, RoundEndAnimationDone));
        }
        else if (_PlayerScore >= _MaxScorePerRound) {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_LoseRound, RoundEndAnimationDone));
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

    public float GetSlapDelay() {
      return Random.Range(_MinSlapDelay, _MaxSlapDelay);
    }

    public void ResetSlapChance() {
      _CurrentSlapChance = _BaseSlapChance;
    }

    protected override int CalculateExcitementStatRewards() {
      return 1 + _CloseRoundCount * 2;
    }

    public void UpdateRoundsUI() {
      // Display the current round
      SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, _CozmoScore + _PlayerScore + 1);
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
