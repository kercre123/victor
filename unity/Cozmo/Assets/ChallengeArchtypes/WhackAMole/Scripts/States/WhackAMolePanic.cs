using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // TODO : In this state, alternate between targets on random intervals with increasing frequency.
  // Returns to normal chase if mole state is not equal to BOTH
  // Plays Frustrated Animation, deactivates both, and sets state to Idle if you remain
  // in this state for too long. Potentially have Cozmo hide or cower until mole state is not
  // equal to BOTH.
  public class WhackAMolePanic : State {
    private WhackAMoleGame _WhackAMoleGame;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }
  }
}
