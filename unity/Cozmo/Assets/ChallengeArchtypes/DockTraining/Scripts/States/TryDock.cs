using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class TryDock : State {

    private LightCube _DockTarget;

    public void Init(LightCube dockTarget) {
      _DockTarget = dockTarget;
    }

    public override void Enter() {
      base.Enter();
      _CurrentRobot.AlignWithObject(_DockTarget, 30.0f, HandleDockCallback);
    }

    private void HandleDockCallback(bool success) {
      AnimationState animationState = new AnimationState();
      if (success) {
        animationState.Initialize(AnimationName.kMajorWin, HandleAnimationDone);
      }
      else {
        animationState.Initialize(AnimationName.kShocked, HandleAnimationDone);
      }
    }

    private void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
    }
  }
}