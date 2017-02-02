using UnityEngine;
using Anki.Cozmo;
using System.Collections.Generic;

namespace SpeedTap {
  public class SpeedTapReactToRoundEnd : State {

    private SpeedTapGame _SpeedTapGame;

    private PlayerInfo _Winner;

    public SpeedTapReactToRoundEnd(PlayerInfo winner) {
      _Winner = winner;
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
      if (_Winner.playerType == PlayerType.Cozmo) {
        winnerPortrait = _SpeedTapGame.SharedMinigameView.CozmoPortrait;
        winnerNameLocKey = LocalizationKeys.kNameCozmo;
      }
      else {
        winnerPortrait = _SpeedTapGame.SharedMinigameView.PlayerPortrait;
        winnerNameLocKey = LocalizationKeys.kNamePlayer;
      }
      roundsWinnerWon = _Winner.playerRoundsWon;
      int roundsNeeded = _SpeedTapGame.RoundsNeededToWin;
      roundEndSlideScript.Initialize(winnerPortrait, winnerNameLocKey, roundsWinnerWon, roundsNeeded, _SpeedTapGame.RoundsPlayed);

      // Play banner animation with score
      string bannerText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextRoundScore,
                            _SpeedTapGame.HumanRoundsWon, _SpeedTapGame.CozmoRoundsWon);
      _SpeedTapGame.SharedMinigameView.PlayBannerAnimation(bannerText, null,
        roundEndSlideScript.BannerAnimationDurationSeconds);

      ContextManager.Instance.AppFlash(playChime: true);
      // Play cozmo animation
      PlayReactToRoundAnimationAndSendEvent();
    }

    public override void Exit() {
      base.Exit();

      // re-enable react to pickup during the next round
      _CurrentRobot.RequestEnableReactionTrigger("speed_tap_anim", ReactionTrigger.RobotPickedUp, true);
      _CurrentRobot.RequestEnableReactionTrigger("speed_tap_anim", ReactionTrigger.ReturnedToTreads, true);
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // COZMO-2033; some of the win game animations cause Cozmo's cliff sensor to trigger
      // So in those cases don't show the "Cozmo Moved; Quit Game" dialog
      // Do nothing
    }

    private void PlayReactToRoundAnimationAndSendEvent() {
      // Don't listen to pickup in case cozmo lifts himself on top of the cube for an animation
      _CurrentRobot.RequestEnableReactionTrigger("speed_tap_anim", ReactionTrigger.RobotPickedUp, false);
      _CurrentRobot.RequestEnableReactionTrigger("speed_tap_anim", ReactionTrigger.ReturnedToTreads, false);

      AnimationTrigger animEvent = AnimationTrigger.Count;
      bool highIntensity = _SpeedTapGame.IsHighIntensityRound();
      if (_Winner.playerType != PlayerType.Cozmo) {
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
      MoveToNextState();
    }

    private void MoveToNextState() {
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();

      _SpeedTapGame.ClearWinningLightPatterns();
      _StateMachine.SetNextState(new SpeedTapCubeSelectionState());
    }
  }
}