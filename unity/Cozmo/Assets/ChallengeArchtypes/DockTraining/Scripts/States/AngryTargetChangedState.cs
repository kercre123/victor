using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class AngryTargetChangedState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation(AnimationName.kShocked, HandleAnimationDone);
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
    }
  }
}


