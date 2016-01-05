using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // TODO : In this state, looks around confused, trying to find cube.
  // If mole state is not NONE, play surprise animation then move to Chase
  // If mole state is NONE for too long, play disappointed animation
  // then return to Idle state. 
  public class WhackAMoleConfusion : State {
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
