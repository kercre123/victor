using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.white);
      LightCube.TappedAction += HandleTap;
    }

    private void HandleTap(int id, int tappedTimes) {
      if (id == _SpeedTapGame.PlayerBlock.ID) {
        _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      }

    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= HandleTap;
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
    }
  }

}
