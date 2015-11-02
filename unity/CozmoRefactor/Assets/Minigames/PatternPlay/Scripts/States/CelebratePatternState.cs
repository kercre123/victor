using UnityEngine;
using System.Collections;

public class CelebratePatternState : State {

  PatternPlayGame patternPlayGame = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("State", "CelebratePattern");
    patternPlayGame = (PatternPlayGame)stateMachine_.GetGame();

    patternPlayGame.ClearBlockLights();

    if (patternPlayGame.ShouldCelebrateNew()) {
      robot.SendAnimation("enjoyPattern", AnimationDone);
      patternPlayGame.SetShouldCelebrateNew(false);
    }
    else {
      robot.SendAnimation("seeOldPattern", AnimationDone);
    }
  }

  void AnimationDone(bool success) {
    patternPlayGame.GetAutoBuild().ClearNeatList();
    patternPlayGame.ResetLookHeadForkLift();
    stateMachine_.SetNextState(new LookForPatternState());
  }
}
