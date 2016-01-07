using System;
using UnityEngine;

namespace Simon {
  public class CozmoTurnToCubeSimonState : State {

    private CozmoSetSequenceSimonState _Parent;

    public override void Enter() {
      base.Enter();
      _Parent = (CozmoSetSequenceSimonState)_StateMachine.GetParentState();
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    public override void Update() {
      base.Update();

      if (TurnToTarget(_Parent.GetCurrentTarget())) {
        _StateMachine.SetNextState(new CozmoBlinkLightsSimonState());
      }
    }

    private bool TurnToTarget(LightCube currentTarget) {
      return SimonGame.TurnToTarget(_CurrentRobot, currentTarget);
    }
  }
}

