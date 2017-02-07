using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapBeginRound : State {
    private SpeedTapGame _SpeedTapGame;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show wide slide
      GameObject roundBeginSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                     _SpeedTapGame.SpeedTapRoundBeginSlidePrefab.gameObject, "speedTap_round_begin_slide");
      SpeedTapRoundBeginSlide roundBeginSlideScript = roundBeginSlide.GetComponent<SpeedTapRoundBeginSlide>();
      roundBeginSlideScript.SetText(_SpeedTapGame.CurrentRound, _SpeedTapGame.TotalRounds, _SpeedTapGame.GetPlayerCount());

      // Play banner animation
      string bannerText = Localization.Get(LocalizationKeys.kSpeedTapTextGetReady);
      _SpeedTapGame.SharedMinigameView.PlayBannerAnimation(bannerText, HandleBannerAnimationEnd);
      _SpeedTapGame.SharedMinigameView.HideShelf();
    }

    private void HandleBannerAnimationEnd() {
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      _SpeedTapGame.ResetScore();
      _StateMachine.SetNextState(new SpeedTapHandCubesOff());
    }

  }
}