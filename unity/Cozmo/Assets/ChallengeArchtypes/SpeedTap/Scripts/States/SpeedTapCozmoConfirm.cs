using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_Tap, HandleTapDone));
    }

    private void HandleTapDone(bool success) {
      _SpeedTapGame.SetCozmoOrigPos();
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
    }
  }

}
