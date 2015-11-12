using UnityEngine;
using System.Collections;

namespace DockTraining {

  public class TrySuccessDockState : State {
    LightCube _CurrentTarget;
    DockTrainingGame _DockTrainingGame;

    public override void Enter() {
      base.Enter();

      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      _DockTrainingGame = _StateMachine.GetGame() as DockTrainingGame;
      _CurrentTarget = _DockTrainingGame.GetCurrentTarget();

      if (_CurrentTarget == null) {
        _StateMachine.SetNextState(new AngryTargetChangedState());
      }
      else {
        _CurrentRobot.AlignWithObject(_CurrentTarget, 20.0f, AlignDone);
      }
    }

    public override void Update() {
      base.Update();

    }

    public override void Exit() {
      base.Exit();
    }

    private void AlignDone(bool success) {
      if (success) {
        _StateMachine.SetNextState(new CelebrateState());
      }
      else {
        _StateMachine.SetNextState(new AngryTargetChangedState());
      }
    }
  }

}
