using UnityEngine;
using System.Collections;

namespace PatternPlay {

  public class CelebratePatternState : State {

    private PatternPlayGame _PatternPlayGame = null;

    public override void Enter() {
      base.Enter();
      DAS.Info("State", "CelebratePattern");
      _PatternPlayGame = (PatternPlayGame)_StateMachine.GetGame();

      _PatternPlayGame.ClearBlockLights();

      if (_PatternPlayGame.ShouldCelebrateNew()) {
        _CurrentRobot.SendAnimation("enjoyPattern", HandleAnimationDone);
        _PatternPlayGame.SetShouldCelebrateNew(false);
      }
      else {
        _CurrentRobot.SendAnimation("seeOldPattern", HandleAnimationDone);
      }
    }

    void HandleAnimationDone(bool success) {
      _PatternPlayGame.GetAutoBuild().ClearNeatList();
      _PatternPlayGame.ResetLookHeadForkLift();
      _StateMachine.SetNextState(new LookForPatternState());
    }
  }

}
