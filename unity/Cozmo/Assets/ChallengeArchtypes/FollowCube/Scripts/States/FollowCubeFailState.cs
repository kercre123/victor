using UnityEngine;
using System.Collections;

namespace FollowCube {
  public class FollowCubeFailState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("shocked", HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new FollowCubeState());
    }
  }

}
