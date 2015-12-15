using UnityEngine;
using System.Collections;

namespace Simon {

  public class CozmoSetSimonState : State {

    private SimonGame _GameInstance;
    private int _CurrentSequenceID;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.PickNewSequence();
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }

  }

}
