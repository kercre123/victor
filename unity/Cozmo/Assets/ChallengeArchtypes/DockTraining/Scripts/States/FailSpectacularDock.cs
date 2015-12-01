using UnityEngine;
using System.Collections;

namespace DockTraining {

  public class FailSpectacularDock : State {
    public override void Enter() {
      base.Enter();
      PlayShockedAnimation();
    }

    private void PlayShockedAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kShocked, HandleAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
    }
  }

}