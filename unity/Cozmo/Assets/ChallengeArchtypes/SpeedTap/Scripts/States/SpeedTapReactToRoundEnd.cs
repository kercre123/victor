using UnityEngine;
using Anki.Cozmo;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapReactToRoundEnd : State {

    private SpeedTapGame _SpeedTapGame;
    private PointWinner _CurrentWinner;

    // INGO See HandleRoundEndAnimDone below
    //private bool _RobotAnimationEnded = false;
    //private bool _BannerAnimationEnded = false;

    public SpeedTapReactToRoundEnd(PointWinner winner) {
      _CurrentWinner = winner;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show round end slide
      GameObject roundEndSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                   _SpeedTapGame.SpeedTapRoundEndSlidePrefab.gameObject, "speedTap_round_end_slide");
      SpeedTapRoundEndSlide roundEndSlideScript = roundEndSlide.GetComponent<SpeedTapRoundEndSlide>();

      // Show current winner
      Sprite winnerPortrait = null;
      string winnerNameLocKey = null;
      int roundsWinnerWon = 0;
      if (_CurrentWinner == PointWinner.Cozmo) {
        winnerPortrait = _SpeedTapGame.SharedMinigameView.CozmoPortrait;
        winnerNameLocKey = LocalizationKeys.kNameCozmo;
        roundsWinnerWon = _SpeedTapGame.CozmoRoundsWon;
      }
      else {
        winnerPortrait = _SpeedTapGame.SharedMinigameView.PlayerPortrait;
        winnerNameLocKey = LocalizationKeys.kNamePlayer;
        roundsWinnerWon = _SpeedTapGame.PlayerRoundsWon;
      }
      int roundsNeeded = _SpeedTapGame.RoundsNeededToWin;
      roundEndSlideScript.Initialize(winnerPortrait, winnerNameLocKey, roundsWinnerWon, roundsNeeded, _SpeedTapGame.RoundsPlayed);

      // Play banner animation with score
      string bannerText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextRoundScore,
                            _SpeedTapGame.PlayerRoundsWon, _SpeedTapGame.CozmoRoundsWon);
      _SpeedTapGame.SharedMinigameView.ShelfWidget.PlayBannerAnimation(bannerText, HandleBannerAnimationDone,
        roundEndSlideScript.BannerAnimationDurationSeconds);

      ContextManager.Instance.AppFlash(playChime: true);
      ContextManager.Instance.HideForeground();
      // Play cozmo animation
      PlayReactToRoundAnimationAndSendEvent();
    }

    public override void Exit() {
      base.Exit();
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // COZMO-2033; some of the win game animations cause Cozmo's cliff sensor to trigger
      // So in those cases don't show the "Cozmo Moved; Quit Game" dialog
      // Do nothing
    }

    private void PlayReactToRoundAnimationAndSendEvent() {
      AnimationTrigger animEvent = AnimationTrigger.Count;
      bool highIntensity = _SpeedTapGame.IsHighIntensityRound();
      if (_CurrentWinner == PointWinner.Player) {
        animEvent = (highIntensity) ?
                  AnimationTrigger.OnSpeedtapRoundPlayerWinHighIntensity : AnimationTrigger.OnSpeedtapRoundPlayerWinLowIntensity;
      }
      else {
        animEvent = (highIntensity) ?
                  AnimationTrigger.OnSpeedtapRoundCozmoWinHighIntensity : AnimationTrigger.OnSpeedtapRoundCozmoWinLowIntensity;
      }
      _CurrentRobot.SendAnimationTrigger(animEvent, HandleRoundEndAnimDone);
    }

    private void HandleRoundEndAnimDone(bool success) {
      // ING0 For now move on to the next state as soon as the robot finishes animating to 
      // avoid "dead space" and don't worry about the banner being on screen.
      MoveToNextState();

      //_RobotAnimationEnded = true;
      //if (_BannerAnimationEnded) {
      //MoveToNextState();
      //}
    }

    private void HandleBannerAnimationDone() {
      // INGO See HandleRoundEndAnimDone above ^
      // _BannerAnimationEnded = true;
      // if (_RobotAnimationEnded) {
      // MoveToNextState();
      // }
    }

    private void MoveToNextState() {
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      _SpeedTapGame.ClearWinningLightPatterns();
      _StateMachine.SetNextState(new SpeedTapCozmoDriveToCube(false));
    }
  }
}