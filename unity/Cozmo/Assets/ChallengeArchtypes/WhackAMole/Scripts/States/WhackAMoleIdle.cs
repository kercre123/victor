using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // TODO: In this state, perform basic idle behaviors until the mole state becomes
  // not equal to NONE. Then enter Chase state. Should only enter this state when
  // both cubes are inactive.
  public class WhackAMoleIdle : State {
    private WhackAMoleGame _WhackAMoleGame;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

