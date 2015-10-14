using UnityEngine;
using System.Collections;

public class LookAtPatternConstruction : State {

  bool arrivedPose = false;

  public override void Enter() {
    base.Enter();
    robot.GotoPose(0.0f, 0.0f, 0.0f, ArrivedPose, false, false);
  }

  public override void Update() {
    base.Update();
    if (arrivedPose) {
      // check to see if pattern is seen
      // if not seen in a bit, try to look for cubes.
    }
  }

  void ArrivedPose(bool success) {
    if (success) {
      arrivedPose = true;
    }
    else {
      stateMachine.SetNextState(new LookForCubes());
    }
  }
}
