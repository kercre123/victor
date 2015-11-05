using UnityEngine;
using System.Collections;

public class LetMeGetItState : State {

  bool animationPlaying_ = false;
  float timeGoldBlockValid_ = 0.0f;
  float timeGoldBlockInvalid_ = 0.0f;

  public override void Enter() {
    base.Enter();
    animationPlaying_ = true;
    robot.SendAnimation("majorWin", AnimationDone);
  }

  public override void Update() {
    base.Update();
    if (animationPlaying_) {
      return;
    }

    if (timeGoldBlockValid_ > 4.0f) {
      // pick up block.
    }

    if (timeGoldBlockInvalid_ > 3.0f) {
      // go back to follow
      stateMachine_.SetNextState(new FollowGoldCubeState());
    }

  }

  public override void Exit() {
    base.Exit();
  }

  private void AnimationDone(bool success) {
    animationPlaying_ = false;
  }

}
