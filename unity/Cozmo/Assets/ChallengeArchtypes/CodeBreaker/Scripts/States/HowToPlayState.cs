using UnityEngine;
using System.Collections;

namespace CodeBreaker {
  public class HowToPlayState : State {

    public override void Enter() {
      base.Enter();
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.ShowHowToPlaySlide(HandleReadyButtonClicked);

      // Play an idle animation on Cozmo; will be inturrupted by 
      // other animations.
      _CurrentRobot.SetIdleAnimation("_LIVE_​");
    }

    public void HandleReadyButtonClicked() {
      _StateMachine.SetNextState(new PickCodeState());
    }
  }
}