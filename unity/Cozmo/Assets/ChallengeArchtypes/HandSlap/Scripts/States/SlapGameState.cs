using UnityEngine;
using System.Collections;

namespace HandSlap {
  public class SlapGameState : State {

    private HandSlapGame _HandSlapGame;

    public override void Enter() {
      base.Enter();
      //_HandSlapGame = (_StateMachine.GetGame() as HandSlapGame);
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

