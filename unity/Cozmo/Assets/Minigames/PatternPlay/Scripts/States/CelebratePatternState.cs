using UnityEngine;
using System.Collections;

public class CelebratePatternState : State {

  private PatternPlayGame _PatternPlayGame = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("State", "CelebratePattern");
    _PatternPlayGame = (PatternPlayGame)stateMachine_.GetGame();

    _PatternPlayGame.ClearBlockLights();

    if (_PatternPlayGame.ShouldCelebrateNew()) {
      robot.SendAnimation("enjoyPattern", AnimationDone);
      _PatternPlayGame.SetShouldCelebrateNew(false);
    }
    else {
      robot.SendAnimation("seeOldPattern", AnimationDone);
    }
  }

  void AnimationDone(bool success) {
    _PatternPlayGame.GetAutoBuild().ClearNeatList();
    _PatternPlayGame.ResetLookHeadForkLift();
    stateMachine_.SetNextState(new LookForPatternState());
  }
}
