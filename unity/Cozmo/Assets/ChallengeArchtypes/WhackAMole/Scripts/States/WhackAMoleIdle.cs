using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // In this state, perform basic idle behaviors until the mole state becomes
  // not equal to NONE. Then enter Chase state. Should only enter this state when
  // both cubes are inactive.
  public class WhackAMoleIdle : State {
    private WhackAMoleGame _WhackAMoleGame;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      _WhackAMoleGame.FixCozmoAngles();
    }

    public override void Update() {
      base.Update();
      if (_WhackAMoleGame.CubeState != WhackAMoleGame.MoleState.NONE) {
        // A cube has been tapped, start chase. If more than one cube is
        // active, Chase will handle moving to Panic.
        _StateMachine.SetNextState(new WhackAMoleChase());
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

