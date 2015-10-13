using UnityEngine;
using System.Collections;

public class CelebratePattern : State {

  PatternPlayController patternPlayController = null;

  public override void Enter() {
    base.Enter();
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();

    if (patternPlayController.SeenNewPattern()) {
      robot.SendAnimation("majorWin", AnimationDone);
    }
    else {
      robot.SendAnimation("minorWin", AnimationDone);
    }

  }

  void AnimationDone(bool success) {
    patternPlayController.ResetLookHeadForkLift();
    stateMachine.SetNextState(new LookForPattern());
  }
}
