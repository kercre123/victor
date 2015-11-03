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

    foreach (KeyValuePair<int, LightCube> lightCube in robot.LightCubes) {
      for (int j = 0; j < lightCube.Value.Lights.Length; ++j) {
        lightCube.Value.Lights[j].onColor = CozmoPalette.ColorToUInt(Color.blue);
      }
    }

    if (stateMachine_.GetGame().robot.LightCubes.Count >= cubesRequired_) {
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
