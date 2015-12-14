using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _DriveTime = 1.0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _SpeedTapGame.CozmoScore = 0;
      _SpeedTapGame.PlayerScore = 0;
      _SpeedTapGame.UpdateUI();
      _CurrentRobot.DriveWheels(20.0f, 20.0f);
    }

    public override void Update() {
      base.Update();
      _DriveTime -= Time.deltaTime;
      if (_DriveTime < 0.0f) {
        AnimationState animation = new AnimationState();
        animation.Initialize(AnimationName.kTapCube, HandleTapDone);
        _StateMachine.SetNextState(animation);
      }

    }

    private void HandleTapDone(bool success) {
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }

}
