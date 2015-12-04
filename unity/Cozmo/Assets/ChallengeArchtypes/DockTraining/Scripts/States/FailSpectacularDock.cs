using UnityEngine;
using System.Collections;

namespace DockTraining {

  public class FailSpectacularDock : State {
    public override void Enter() {
      base.Enter();
    }

    public override void Update() {
      base.Update();
      PlayShockedAnimation();
    }

    private void PlayShockedAnimation() {
      Debug.Log("FAIL SPECTACULAR");
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kShocked, HandleAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
    }
  }

}