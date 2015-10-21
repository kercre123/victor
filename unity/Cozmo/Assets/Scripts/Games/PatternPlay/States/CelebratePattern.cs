using UnityEngine;
using System.Collections;

public class CelebratePattern : State {

  PatternPlayController patternPlayController = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("State", "CelebratePattern");
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();

    patternPlayController.ClearBlockLights();

    if (patternPlayController.ShouldCelebrateNew()) {
      robot.SendAnimation("enjoyPattern", AnimationDone);
      patternPlayController.SetShouldCelebrateNew(false);
    }
    else {
      robot.SendAnimation("seeOldPattern", AnimationDone);
    }
  }

  void AnimationDone(bool success) {
    patternPlayController.ResetLookHeadForkLift();
    stateMachine.SetNextState(new LookForPattern());
  }
}
