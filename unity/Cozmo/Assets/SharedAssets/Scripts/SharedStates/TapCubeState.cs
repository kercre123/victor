using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class TapCubeState : State {

  private State _NextState;
  private int _CubeId;

  public delegate void CubeStateDone();

  CubeStateDone _CubeStateDone = null;

  public TapCubeState(State nextState, int cubeId, CubeStateDone cubeStateDone = null) {
    _NextState = nextState;
    _CubeId = cubeId;
    _CubeStateDone = cubeStateDone;
  }

  public override void Enter() {
    base.Enter();

    LightCube.TappedAction += HandleTap;
  }

  private void HandleTap(int id, int tapCount, float timeStamp) {
    if (id == _CubeId) {
      if (_CubeStateDone != null) {
        _CubeStateDone();
      }

      _StateMachine.SetNextState(_NextState);
      LightCube.TappedAction -= HandleTap;
    }
  }


  public override void Exit() {
    base.Exit();
    LightCube.TappedAction -= HandleTap;
  }
}
