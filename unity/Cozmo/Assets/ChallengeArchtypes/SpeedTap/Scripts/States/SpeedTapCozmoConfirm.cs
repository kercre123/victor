using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;
    private const float _kAdjustTime = 1.25f;
    private const float _kAdjustSpeed = 25.0f;
    private float _adjustTimer = 0.0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _adjustTimer = _kAdjustTime;
      _CurrentRobot.DriveWheels(_kAdjustSpeed, _kAdjustSpeed);
    }

    public override void Update() {
      base.Update();
      if (_adjustTimer > 0.0f) {
        _adjustTimer -= Time.deltaTime;
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        _StateMachine.SetNextState(new AnimationState(_SpeedTapGame.RandomTap(), HandleTapDone));
      }
    }

    private void HandleTapDone(bool success) {
      _SpeedTapGame.SetCozmoOrigPos();
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.SetLiftHeight(1.0f);
    }
  }

}
