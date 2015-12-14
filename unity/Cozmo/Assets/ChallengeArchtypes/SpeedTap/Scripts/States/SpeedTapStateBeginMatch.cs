using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapStateBeginMatch : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _DriveTime = 1.0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      // Do a little dance
      // Blink some lights
      // Clear the score
      _SpeedTapGame.CozmoScore = 0;
      _SpeedTapGame.PlayerScore = 0;
      _SpeedTapGame.UpdateUI();
      _CurrentRobot.DriveWheels(20.0f, 20.0f);
    }

    public override void Update() {
      base.Update();
      _DriveTime -= Time.deltaTime;
      if (_DriveTime < 0.0f) {
        _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      }

    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }

}
