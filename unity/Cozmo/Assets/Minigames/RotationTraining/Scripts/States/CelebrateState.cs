using UnityEngine;
using System.Collections;

namespace RotationTraining {
  public class CelebrateState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("majorWin", HandleAnimationDone);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }

    private void HandleAnimationDone(bool success) {
      _CurrentRobot.TurnInPlace(Random.Range(-90, 90), TurnInPlaceDone);
    }

    private void TurnInPlaceDone(bool success) {
      _StateMachine.SetNextState(new RotateState());
    }
  }

}
