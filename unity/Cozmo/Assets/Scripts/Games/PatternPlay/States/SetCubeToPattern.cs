using UnityEngine;
using System.Collections;

public class SetCubeToPattern : State {
  public override void Enter() {
    base.Enter();
    stateMachine.SetNextState(new PlaceCube());
  }
}
