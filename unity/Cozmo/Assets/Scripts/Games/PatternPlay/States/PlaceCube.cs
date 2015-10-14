using UnityEngine;
using System.Collections;

public class PlaceCube : State {
  public override void Enter() {
    base.Enter();
    //robot.PlaceObjectOnGround();
  }

  void PlaceDone(bool success) {
    if (success) {
      stateMachine.SetNextState(new LookAtPatternConstruction());
    }
    else {
      stateMachine.SetNextState(new LookForCubes());
    }
  }
}
