using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InitialCubesState : State {

  State nextState_;
  int cubesRequired_;

  public delegate void CubeStateDone();

  CubeStateDone cubeStateDone_ = null;

  public void InitialCubeRequirements(State nextState, int cubesRequired, CubeStateDone cubeStateDone) {
    nextState_ = nextState;
    cubesRequired_ = cubesRequired;
    cubeStateDone_ = cubeStateDone;
  }

  public override void Enter() {
    base.Enter();
  }

  public override void Update() {
    base.Update();

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.ActiveBlocks) {
      for (int j = 0; j < activeBlock.Value.Lights.Length; ++j) {
        activeBlock.Value.Lights[j].onColor = CozmoPalette.ColorToUInt(Color.blue);
      }
    }

    if (stateMachine_.GetGame().robot.ActiveBlocks.Count >= cubesRequired_) {
      stateMachine_.SetNextState(nextState_);
    }
  }

  public override void Exit() {
    base.Exit();
    if (cubeStateDone_ != null) {
      cubeStateDone_();
    }
  }
}
