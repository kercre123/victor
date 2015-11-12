using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class AngryTargetChangedState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("MinorIrritation", AnimationDone);
    }

    private void AnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
    }
  }
}


