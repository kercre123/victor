using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace PatternGuess {
  public class PickCodeState : State {
    private CubeCode _Code;
    private LightCube[] _TargetCubes;

    public PickCodeState(LightCube[] targetCubes) {
      _TargetCubes = targetCubes;
    }

    public override void Enter() {
      base.Enter();

      // Update the UI
      PatternGuessGame game = _StateMachine.GetGame() as PatternGuessGame;
      game.ShowReadySlide(LocalizationKeys.kPatternGuessTextHowToPlayLong,
        LocalizationKeys.kPatternGuessButtonPickingCode, null);
      game.DisableReadyButton();
      game.ResetGuesses();
      _Code = game.GetRandomCode();

      // TODO: Turn towards the center of the cubes to emulate "thinking"
      // Play a think animation on Cozmo
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.PatternGuessThinking, HandleThinkAnimationDone);

      foreach (var cube in _TargetCubes) {
        cube.SetFlashingLEDs(Color.white);
      }
    }

    public void HandleThinkAnimationDone(bool success) {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.PatternGuessNewIdea, HandleAhaAnimationDone);
    }

    public void HandleAhaAnimationDone(bool success) {
      GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
      // Move on to the next state
      _StateMachine.SetNextState(new WaitForGuessState(_Code, _TargetCubes));
    }
  }
}