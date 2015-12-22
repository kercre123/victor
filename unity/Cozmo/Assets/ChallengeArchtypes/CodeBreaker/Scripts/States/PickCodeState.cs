using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CodeBreaker {
  public class PickCodeState : State {
    private CubeCode _Code;
    private LightCube[] _TargetCubes;

    public PickCodeState(LightCube[] targetCubes) {
      _TargetCubes = targetCubes;
    }

    public override void Enter() {
      base.Enter();

      // Update the UI
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.ShowReadySlide(LocalizationKeys.kCodeBreakerTextHowToPlayLong, 
        LocalizationKeys.kCodeBreakerButtonPickingCode, null);
      game.DisableReadyButton();
      game.ResetGuesses();
      _Code = game.GetRandomCode();

      // TODO: Play a think animation on Cozmo
      _CurrentRobot.SendAnimation(AnimationName.kEnjoyPattern, HandleAnimationDone);

      foreach (var cube in _TargetCubes) {
        cube.SetFlashingLEDs(Color.white);
      }
    }

    public void HandleAnimationDone(bool success) {
      // Move on to the next state
      _StateMachine.SetNextState(new WaitForGuessState(_Code, _TargetCubes));
    }
  }
}