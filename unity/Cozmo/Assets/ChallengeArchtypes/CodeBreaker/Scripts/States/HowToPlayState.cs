using UnityEngine;
using System.Collections;

namespace CodeBreaker {
  public class HowToPlayState : State {

    public override void Enter() {
      base.Enter();
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.ShowHowToPlaySlide(HandleReadyButtonClicked);
    }

    public void HandleReadyButtonClicked() {
      _StateMachine.SetNextState(new PickCodeState());
    }
  }
}