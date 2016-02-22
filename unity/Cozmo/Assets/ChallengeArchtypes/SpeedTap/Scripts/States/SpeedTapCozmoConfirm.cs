using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _DriveTime = 0.0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.CozmoAdjust();
    }

    public override void Update() {
      base.Update();
      if (_SpeedTapGame.CozmoAdjustTimeLeft > 0.0f) {
        _SpeedTapGame.CozmoAdjustTimeLeft -= Time.deltaTime;
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        _StateMachine.SetNextState(new AnimationState(_SpeedTapGame.RandomTap(), HandleTapDone));
      }
    }

    private void HandleTapDone(bool success) {
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.SetLiftHeight(1.0f);
    }
  }

}
