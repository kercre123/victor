using System;
using UnityEngine;

namespace Simon {
  public class CozmoTurnToCubeSimonState : State {

    private LightCube _TargetCube;

    public CozmoTurnToCubeSimonState(LightCube targetCube) {
      _TargetCube = targetCube;
    }

    public override void Enter() {
      base.Enter();
      _CurrentRobot.TurnTowardsObject(_TargetCube, false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2, HandleTurnFinished);
    }

    private void HandleTurnFinished(bool success) {
      _StateMachine.SetNextState(new CozmoBlinkLightsSimonState(_TargetCube));
    }
  }
}

