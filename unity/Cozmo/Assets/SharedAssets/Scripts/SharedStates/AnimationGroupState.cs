using UnityEngine;
using System.Collections;

public class AnimationGroupState : State {

  public delegate void AnimationDoneHandler(bool success);

  private Anki.Cozmo.AnimationTrigger _AnimationTrigger;
  private AnimationDoneHandler _AnimationFinishedCallback;

  public AnimationGroupState(Anki.Cozmo.AnimationTrigger trigger, AnimationDoneHandler animationFinishedCallback) {
    _AnimationTrigger = trigger;
    _AnimationFinishedCallback = animationFinishedCallback;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.SendAnimationTrigger(_AnimationTrigger, HandleAnimationDone);
  }

  public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    // Do nothing
  }

  private void HandleAnimationDone(bool success) {
    if (_AnimationFinishedCallback != null) {
      _AnimationFinishedCallback(success);
    }
  }
}
