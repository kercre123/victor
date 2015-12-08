using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class WaitForTargetState : State {

    private DockTrainingGame _DockTrainingGame;

    public override void Enter() {
      base.Enter();
      _DockTrainingGame = _StateMachine.GetGame() as DockTrainingGame;
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
    }

    public override void Update() {
      base.Update();

      LightCube target = _DockTrainingGame.GetCurrentTarget();
      if (target != null) {

        var tapState = new TapCubeState(new DetermineNextAction(), target.ID);
        _StateMachine.SetNextState(tapState);
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

