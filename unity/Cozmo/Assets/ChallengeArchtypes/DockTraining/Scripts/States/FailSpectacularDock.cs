using UnityEngine;
using System.Collections;

namespace DockTraining {

  public class FailSpectacularDock : State {

    private DockTrainingGame _GameInstance;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as DockTrainingGame;
    }

    public override void Update() {
      base.Update();
      PlayShockedAnimation();
      _CurrentRobot.DriveWheels(25.0f, 10.0f);
    }

    public override void Exit() {
      base.Exit();
    }

    private void PlayShockedAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kShocked, HandleAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void HandleAnimationDone(bool success) {
      _CurrentRobot.GotoPose(_GameInstance.StartingPosition(), _GameInstance.StartingRotation(), HandleBackToStartPos);
    }

    private void HandleBackToStartPos(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
      (_StateMachine.GetGame() as DockTrainingGame).DockFailed();
    }
  }

}