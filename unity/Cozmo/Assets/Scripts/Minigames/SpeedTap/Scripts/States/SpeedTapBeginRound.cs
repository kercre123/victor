using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapBeginRound : State {
    private SpeedTapGame _SpeedTapGame;
    const float kWaitTime = 2.5f;
    public float timerStart;
    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show wide slide
      GameObject roundBeginSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                     _SpeedTapGame.SpeedTapRoundBeginSlidePrefab.gameObject, "speedTap_round_begin_slide");
      SpeedTapRoundBeginSlide roundBeginSlideScript = roundBeginSlide.GetComponent<SpeedTapRoundBeginSlide>();
      roundBeginSlideScript.SetText(_SpeedTapGame.CurrentRound, _SpeedTapGame.TotalRounds, _SpeedTapGame.GetPlayerCount());

      _SpeedTapGame.SharedMinigameView.HideShelf();
      timerStart = Time.time;

    }

    public override void Update() {
      base.Update();
      if (Time.time >= timerStart + kWaitTime) {
        HandleTimerOver();
      }
    }

    private void HandleTimerOver() {
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      _SpeedTapGame.ResetScore();
      _StateMachine.SetNextState(new SpeedTapHandCubesOff());
    }

  }
}