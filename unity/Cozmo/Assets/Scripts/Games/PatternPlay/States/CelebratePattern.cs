using UnityEngine;
using System.Collections;

public class CelebratePattern : State {
  public override void Enter() {
    base.Enter();
    robot.SendAnimation("majorWin", AnimationDone);
  }

  void AnimationDone(bool success) {
    stateMachine.SetNextState(new LookForPattern());
  }
}
