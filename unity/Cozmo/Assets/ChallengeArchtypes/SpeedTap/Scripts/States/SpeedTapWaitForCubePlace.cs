using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapWaitForCubePlace : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.white);
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
    }

    public override void Update() {
      base.Update();
      if (_SpeedTapGame.CozmoBlock.MarkersVisible) {
        float distance = Vector3.Distance(_CurrentRobot.WorldPosition, _SpeedTapGame.CozmoBlock.WorldPosition);
        if (distance < 60.0f) {
          _StateMachine.SetNextState(new SpeedTapStateBeginMatch());
        }
      }
    }
  }

}
