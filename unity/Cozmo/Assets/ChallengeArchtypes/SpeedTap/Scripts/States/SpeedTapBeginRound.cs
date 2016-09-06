using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapBeginRound : State {
    private SpeedTapGame _SpeedTapGame;

    public override void Enter() {
      base.Enter();
      // This is here rather than the Handle Light Cube Tap event in Player Confirm
      // in order to fix an issue where the claim cube sound could be repeatedly triggered
      // with the state machine paused.
      // Ben wants to use the same sound for player tap and for Cozmo tap
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Cube_Cozmo_Tap);
      ContextManager.Instance.AppHoldStart(anticipation: true);
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show wide slide
      GameObject roundBeginSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                     _SpeedTapGame.SpeedTapRoundBeginSlidePrefab.gameObject, "speedTap_round_begin_slide");
      SpeedTapRoundBeginSlide roundBeginSlideScript = roundBeginSlide.GetComponent<SpeedTapRoundBeginSlide>();
      roundBeginSlideScript.SetText(_SpeedTapGame.CurrentRound, _SpeedTapGame.TotalRounds);

      // Play banner animation
      string bannerText = Localization.Get(LocalizationKeys.kSpeedTapTextGetReady);
      _SpeedTapGame.SharedMinigameView.PlayBannerAnimation(bannerText, HandleBannerAnimationEnd);
      _SpeedTapGame.SharedMinigameView.HideShelf();
    }

    private void HandleBannerAnimationEnd() {
      ContextManager.Instance.AppHoldEnd();
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      _SpeedTapGame.ResetScore();
      ContextManager.Instance.ShowForeground();
      _StateMachine.SetNextState(new SpeedTapHandCubesOff());
    }

    public override void Exit() {
      base.Exit();
    }
  }
}