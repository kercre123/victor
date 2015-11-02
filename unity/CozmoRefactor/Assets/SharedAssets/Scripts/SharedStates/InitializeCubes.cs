using UnityEngine;
using System.Collections;

public class InitializeCubes : State {

  State nextState_;
  int cubesRequired_;

  public void InitialCubeRequirements(State nextState, int cubesRequired) {
    nextState_ = nextState;
    cubesRequired_ = cubesRequired;
  }

  public override void Enter() {
    base.Enter();
  }

  public override void Update() {
    base.Update();
    if (stateMachine_.GetGame().GetRobot.ActiveBlocks.Count >= cubesRequired_) {
      stateMachine_.SetNextState(nextState_);
    }
  }

  public override void Exit() {
    base.Exit();
  }
}
