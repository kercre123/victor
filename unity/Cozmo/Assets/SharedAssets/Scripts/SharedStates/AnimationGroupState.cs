using UnityEngine;
using System.Collections;
using AnimationGroups;

public class AnimationGroupState : State {

  public delegate void AnimationDoneHandler(bool success);

  private string _AnimationGroupName;
  private AnimationDoneHandler _AnimationFinishedCallback;

  public AnimationGroupState(string animationGroupName, AnimationDoneHandler animationFinishedCallback) {
    _AnimationGroupName = animationGroupName;
    _AnimationFinishedCallback = animationFinishedCallback;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.SendAnimationGroup(_AnimationGroupName, HandleAnimationDone);
  }

  private void HandleAnimationDone(bool success) {
    if (_AnimationFinishedCallback != null) {
      _AnimationFinishedCallback(success);
    }
  }
}
