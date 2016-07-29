using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

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

      // TODO: Turn towards the center of the cubes to emulate "thinking"
      // Play a think animation on Cozmo
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CodeBreakerThinking, HandleThinkAnimationDone);

      foreach (var cube in _TargetCubes) {
        cube.SetFlashingLEDs(Color.white);
      }
    }

    public void HandleThinkAnimationDone(bool success) {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CodeBreakerNewIdea, HandleAhaAnimationDone);
    }

    public void HandleAhaAnimationDone(bool success) {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Cozmo_Connect);
      // Move on to the next state
      _StateMachine.SetNextState(new WaitForGuessState(_Code, _TargetCubes));
    }
  }
}