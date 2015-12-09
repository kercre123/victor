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
        target.SetLEDs(Color.blue);
        _StateMachine.SetNextState(new TapCubeState(new DetermineNextAction(), target.ID));
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

