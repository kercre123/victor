using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _DriveTime = 1.5f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _CurrentRobot.DriveWheels(25.0f, 25.0f);
    }

    public override void Update() {
      base.Update();
      _DriveTime -= Time.deltaTime;
      if (_DriveTime < 0.0f) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kSpeedTap_Tap_01, HandleTapDone));
      }
    }

    private void HandleTapDone(bool success) {
      _SpeedTapGame.PlayPos = _CurrentRobot.WorldPosition;
      _SpeedTapGame.PlayRot = _CurrentRobot.Rotation;
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.SetLiftHeight(1.0f);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }

}
