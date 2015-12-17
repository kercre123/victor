using UnityEngine;
using System.Collections;

namespace CodeBreaker {
  public class PickCodeState : State {
    public override void Enter() {
      base.Enter();
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.DisableButton();
      game.SetButtonText(Localization.Get(LocalizationKeys.kCodeBreakerButtonPickingCode));
    }
  }
}