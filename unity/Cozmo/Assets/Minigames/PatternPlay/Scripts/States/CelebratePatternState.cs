using UnityEngine;
using System.Collections;

public class CelebratePatternState : State {

  private PatternPlayGame patternPlayGame_ = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("State", "CelebratePattern");
    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();

    patternPlayGame_.ClearBlockLights();

    if (patternPlayGame_.ShouldCelebrateNew()) {
      robot.SendAnimation("enjoyPattern", AnimationDone);
      patternPlayGame_.SetShouldCelebrateNew(false);
    }
    else {
      robot.SendAnimation("seeOldPattern", AnimationDone);
    }
  }

  void AnimationDone(bool success) {
    patternPlayGame_.GetAutoBuild().ClearNeatList();
    patternPlayGame_.ResetLookHeadForkLift();
    stateMachine_.SetNextState(new LookForPatternState());
  }
}
