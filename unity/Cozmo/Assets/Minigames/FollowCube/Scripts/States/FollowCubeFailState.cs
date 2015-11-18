using UnityEngine;
using System.Collections;

namespace FollowCube {
  public class FollowCubeFailState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("shocked", HandleAnimationDone);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new FollowCubeState());
    }
  }

}
