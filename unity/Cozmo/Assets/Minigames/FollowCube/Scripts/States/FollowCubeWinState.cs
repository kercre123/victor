using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeWinState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("majorWin", HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new FollowCubeState());
    }
  }

}
