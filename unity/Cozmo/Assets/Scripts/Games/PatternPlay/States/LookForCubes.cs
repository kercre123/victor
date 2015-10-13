using UnityEngine;
using System.Collections;

public class LookForCubes : State {
  int cubeSeenCount;

  public override void Enter() {
    base.Enter();
  }

  public override void Update() {
    base.Update();
    if (cubeSeenCount > 0) {
      stateMachine.SetNextState(new PickUpCube());
    }
  }

  public override void Exit() {
    base.Exit();
  }

}
