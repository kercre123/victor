using UnityEngine;
using System.Collections;

namespace DockTraining {

  public class FailSpectacularDock : State {
    public override void Enter() {
      base.Enter();
    }

    public override void Update() {
      base.Update();
      _CurrentRobot.DriveWheels(25.0f, 10.0f);
    }

    private void PlayShockedAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kShocked, HandleAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void HandleAnimationDone(bool success) {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _StateMachine.SetNextState(new WaitForTargetState());
    }
  }

}