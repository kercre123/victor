using UnityEngine;
using System.Collections;

public class AnimationState : State {

  public delegate void AnimationDoneHandler(bool success);

  private string _AnimationName;
  private AnimationDoneHandler _AnimationFinishedCallback;

  public void Initialize(string animationName, AnimationDoneHandler animationFinishedCallback) {
    _AnimationName = animationName;
    _AnimationFinishedCallback = animationFinishedCallback;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.SendAnimation(_AnimationName, HandleAnimationDone);
  }

  private void HandleAnimationDone(bool success) {
    if (_AnimationFinishedCallback != null) {
      _AnimationFinishedCallback(success);
    }
  }
}
