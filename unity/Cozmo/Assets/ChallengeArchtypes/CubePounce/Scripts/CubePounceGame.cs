using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;
using System.Collections.Generic;

namespace CubePounce {
  // TODO : SCORE RELATED LOGIC HAS BEEN MOVED TO THE SCORING REGION OF GAMEBASE, private fields for score/rounds are obsolete
  public class CubePounceGame : GameBase {

    public const string kSetUp = "SetUp";
    public const string kWaitForPounce = "WaitForPounce";
    public const string kCozmoWinEarly = "CozmoWinEarly";
    public const string kCozmoWinPounce = "CozmoWinPounce";
    public const string kPlayerWin = "PlayerWin";
    // Consts for determining the exact placement and forgiveness for cube location
    // Must be consistent for animations to work
    public const float kCubePlaceDist = 65.0f;
    private int _CloseRoundCount;

    private float _MinAttemptDelay;
    private float _MaxAttemptDelay;

    private bool _CliffFlagTrown = false;
    private float _CurrentPounceChance;
    private float _BasePounceChance;
    private int _MaxFakeouts;

    private MusicStateWrapper _BetweenRoundsMusic;

    private LightCube _CurrentTarget = null;

    public bool AllRoundsCompleted {
      get {
        int losingScore = Mathf.Min(PlayerRoundsWon, CozmoRoundsWon);
        int winningScore = Mathf.Max(PlayerRoundsWon, CozmoRoundsWon);
        int roundsLeft = TotalRounds - losingScore - winningScore;
        return (winningScore > losingScore + roundsLeft);
      }
    }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      CubePounceConfig config = minigameConfig as CubePounceConfig;
      TotalRounds = config.Rounds;
      _MinAttemptDelay = config.MinAttemptDelay;
      _MaxAttemptDelay = config.MaxAttemptDelay;
      _BasePounceChance = config.StartingPounceChance;
      _MaxFakeouts = config.MaxFakeouts;
      MaxScorePerRound = config.MaxScorePerRound;
      CozmoScore = 0;
      PlayerScore = 0;
      PlayerRoundsWon = 0;
      CozmoRoundsWon = 0;
      _CurrentTarget = null;
      _BetweenRoundsMusic = config.BetweenRoundMusic;
      InitializeMinigameObjects(config.NumCubesRequired());
    }

    protected void InitializeMinigameObjects(int numCubes) {

      CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.CliffEvent), HandleCliffEvent);

      InitialCubesState initCubeState = new InitialCubesState(new SeekState(), numCubes);
      _StateMachine.SetNextState(initCubeState);
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.CliffEvent), HandleCliffEvent);
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
    public bool AttemptPounce() {
      bool didPounce = false;
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
        didPounce = true;
      }
      else {
        // If you do a fakeout instead, increase the likelyhood of a slap
        // attempt based on the max number of fakeouts.
        _CurrentPounceChance += ((1.0f - _BasePounceChance) / _MaxFakeouts);
        CurrentRobot.SendAnimationGroup(AnimationGroupName.kCubePounce_Fake, HandleFakeoutEnd);
      }
      return didPounce;
    }

    private void HandlePounceEnd(bool success) {
      // If the animation completes and the cube is beneath Cozmo,
      // Cozmo has won.
      if (_CliffFlagTrown) {
        SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinPoint);
        SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinPoint);
        OnCozmoWin();
        return;
      }
      else {
        // If the animation completes Cozmo is not on top of the Cube,
        // The player has won this round 
        SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPlayerWinPoint);
        SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoPlayerWinPoint);
        OnPlayerWin();
      }
    }

    public void HandleFakeoutEnd(bool success) {
      _StateMachine.SetNextState(new PounceState());
    }

    // TODO: Replace HandleCliffEvent with a better way to identify if Cozmo has successfully pulled off a pounce.
    // Currently, Cozmo will trigger the end of their pounce animation before the cliff event is registered, possibly
    // can have a quick fix here by adding more to the end of the pounce animation, although much more likely we will
    // have to replace this entirely.
    private void HandleCliffEvent(object messageObject) {
      _CliffFlagTrown = true;
    }

    public void OnPlayerWin() {
      PlayerScore++;
      UpdateScoreboard();
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedWin);
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_LoseHand, HandEndAnimationDone));
    }

    public void OnCozmoWin() {
      CozmoScore++;
      UpdateScoreboard();
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedLose);
      CurrentRobot.CancelCallback(HandleFakeoutEnd);
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_WinHand, HandEndAnimationDone));
    }

    public void HandEndAnimationDone(bool success) {
      // Determines winner and loser at the end of Cozmo's animation, or returns
      // to seek state for the next round
      // Display the current round
      if (Mathf.Max(CozmoScore, PlayerScore) > MaxScorePerRound) {
        if (CozmoScore > PlayerScore) {
          CozmoRoundsWon++;
        }
        else {
          PlayerRoundsWon++;
        }
        UpdateScoreboard();
        if (AllRoundsCompleted) {
          GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
          if (CozmoRoundsWon > PlayerRoundsWon) {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_WinSession, HandleLoseGameAnimationDone));
          }
          else {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_LoseSession, HandleWinGameAnimationDone));
          }
        }
        else {
          GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedRoundEnd);
          GameAudioClient.SetMusicState(_BetweenRoundsMusic);
          if (CozmoScore > PlayerScore) {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_WinRound, RoundEndAnimationDone));
          }
          else {
            _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kCubePounce_LoseRound, RoundEndAnimationDone));
          }
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
      if (Mathf.Abs(PlayerScore - CozmoScore) < 2) {
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

    protected override void RaiseMiniGameQuit() {
      base.RaiseMiniGameQuit();

      Dictionary<string, string> quitGameScoreKeyValues = new Dictionary<string, string>();
      Dictionary<string, string> quitGameRoundsWonKeyValues = new Dictionary<string, string>();

      quitGameScoreKeyValues.Add("CozmoScore", CozmoScore.ToString());
      quitGameRoundsWonKeyValues.Add("CozmoRoundsWon", CozmoRoundsWon.ToString());

      DAS.Event(DASConstants.Game.kQuitGameScore, PlayerScore.ToString(), null, quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, PlayerRoundsWon.ToString(), null, quitGameRoundsWonKeyValues);
    }

  }
}
