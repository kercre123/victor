using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InitialCubesState : State {

  private State _NextState;
  private int _CubesRequired;

  public delegate void CubeStateDone();

  CubeStateDone _CubeStateDone = null;

  public void InitialCubeRequirements(State nextState, int cubesRequired, CubeStateDone cubeStateDone) {
    _NextState = nextState;
    _CubesRequired = cubesRequired;
    _CubeStateDone = cubeStateDone;
  }

  public override void Enter() {
    base.Enter();
  }

  public override void Update() {
    base.Update();

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      for (int j = 0; j < lightCube.Value.Lights.Length; ++j) {
        lightCube.Value.Lights[j].OnColor = CozmoPalette.ColorToUInt(Color.blue);
      }
    }

    if (_StateMachine.GetGame().robot.LightCubes.Count >= _CubesRequired) {
      _StateMachine.SetNextState(_NextState);
    }
  }

  public override void Exit() {
    base.Exit();
    if (_CubeStateDone != null) {
      _CubeStateDone();
    }
  }
}
