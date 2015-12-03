using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class WaitForTargetState : State {

    private DockTrainingGame _DockTrainingGame;

    public override void Enter() {
      base.Enter();
      _DockTrainingGame = _StateMachine.GetGame() as DockTrainingGame;
    }

    public override void Update() {
      base.Update();

      LightCube target = _DockTrainingGame.GetCurrentTarget();
      if (target != null) {
        _StateMachine.SetNextState(new DetermineNextAction());
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

