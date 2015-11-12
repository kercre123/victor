using UnityEngine;
using System.Collections;

namespace RotationTraining {
  public class CelebrateState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("majorWin", AnimationDone);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }

    private void AnimationDone(bool success) {
      _CurrentRobot.TurnInPlace(180.0f, TurnInPlaceDone);
    }

    private void TurnInPlaceDone(bool success) {
      _StateMachine.SetNextState(new RotateState());
    }
  }

}
