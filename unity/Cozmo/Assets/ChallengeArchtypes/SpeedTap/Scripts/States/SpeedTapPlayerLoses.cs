using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerLoses : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.red);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }
  }

}
