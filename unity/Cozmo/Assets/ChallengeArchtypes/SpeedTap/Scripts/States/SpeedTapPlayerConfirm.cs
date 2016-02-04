using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    private string kSlideName = "TellPlayerToTap";

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.white);
      LightCube.TappedAction += HandleTap;
      _SpeedTapGame.ShowGameStateSlide(kSlideName);

    }

    private void HandleTap(int id, int tappedTimes) {
      if (id == _SpeedTapGame.PlayerBlock.ID) {
        _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      }

    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.HideGameStateSlide();
      LightCube.TappedAction -= HandleTap;
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _SpeedTapGame.ResetScore();
    }
  }

}
