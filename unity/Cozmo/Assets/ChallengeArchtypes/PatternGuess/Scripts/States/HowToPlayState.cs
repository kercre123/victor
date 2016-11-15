using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace PatternGuess {
  public class HowToPlayState : State {

    private LightCube[] _TargetCubes;

    public override void Enter() {
      base.Enter();
      PatternGuessGame game = _StateMachine.GetGame() as PatternGuessGame;
      game.ShowReadySlide(LocalizationKeys.kPatternGuessTextHowToPlayLong,
        LocalizationKeys.kButtonReady, HandleReadyButtonClicked);

      _CurrentRobot.TurnOffAllLights();
      List<LightCube> targetCubes = new List<LightCube>();
      foreach (ObservedObject obj in _CurrentRobot.VisibleLightCubes) {
        targetCubes.Add(obj as LightCube);

        if (targetCubes.Count >= game.NumCubesInCode) {
          break;
        }
      }
      _TargetCubes = targetCubes.ToArray();
    }

    public void HandleReadyButtonClicked() {
      _StateMachine.SetNextState(new PickCodeState(_TargetCubes));
    }
  }
}