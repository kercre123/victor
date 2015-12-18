using UnityEngine;
using System.Collections;

namespace CodeBreaker {
  public class PickCodeState : State {
    private CubeCode _Code;

    public override void Enter() {
      base.Enter();

      // Update the UI
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.DisableReadyButton();
      game.SetReadyButtonText(Localization.Get(LocalizationKeys.kCodeBreakerButtonPickingCode));
      _Code = game.GetRandomCode();

      // TODO: Play a think animation on Cozmo
      _CurrentRobot.SendAnimation(AnimationName.kHeadDown, HandleAnimationDone);
    }

    public void HandleAnimationDone(bool success) {
      // Move on to the next state
      _StateMachine.SetNextState(new WaitForGuessState(_Code));
    }
  }
}