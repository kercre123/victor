using UnityEngine;
using System.Collections;

public class HaveIdeaForPattern : State {

  public override void Enter() {
    base.Enter();
    // TODO: pick a pattern to build.
    // TODO: Have idea animation
    robot.SendAnimation("majorWin", AnimationDone);
  }

  private void AnimationDone(bool success) {
    // TODO: go to look for cubes state.
    stateMachine.SetNextState(new LookForCubes());
  }
}
