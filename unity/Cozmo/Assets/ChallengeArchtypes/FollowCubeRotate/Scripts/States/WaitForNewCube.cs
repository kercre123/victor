using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class WaitForNewCube : State {
    private FollowCubeRotateGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as FollowCubeRotateGame;
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
    }

    public override void Update() {
      base.Update();
      _Game.FindNewCubeTarget();
      if (_Game.CurrentTarget != null) {
        _StateMachine.SetNextState(new RotateWithTarget());
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

