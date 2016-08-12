using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapBeginRound : State {
    private SpeedTapGame _SpeedTapGame;

    public override void Enter() {
      base.Enter();
      ContextManager.Instance.AppHoldStart(anticipation: true);
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show wide slide
      GameObject roundBeginSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                     _SpeedTapGame.SpeedTapRoundBeginSlidePrefab.gameObject, "speedTap_round_begin_slide");
      SpeedTapRoundBeginSlide roundBeginSlideScript = roundBeginSlide.GetComponent<SpeedTapRoundBeginSlide>();
      roundBeginSlideScript.SetText(_SpeedTapGame.CurrentRound, _SpeedTapGame.TotalRounds);

      // Play banner animation
      string bannerText = Localization.Get(LocalizationKeys.kSpeedTapTextGetReady);
      _SpeedTapGame.SharedMinigameView.ShelfWidget.PlayBannerAnimation(bannerText, HandleBannerAnimationEnd);
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