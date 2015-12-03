using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class WaitForNewCube : State {
    private FollowCubeRotateGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as FollowCubeRotateGame;

    }

    public override void Update() {
      base.Update();
      LightCube newTarget = _Game.FindNewCubeTarget();
      if (newTarget != null) {
        RotateWithTarget rotateState = new RotateWithTarget();
        rotateState.SetTarget(newTarget);
        _StateMachine.SetNextState(rotateState);
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

