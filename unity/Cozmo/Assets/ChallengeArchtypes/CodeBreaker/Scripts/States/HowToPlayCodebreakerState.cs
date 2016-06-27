using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CodeBreaker {
  public class HowToPlayCodebreakerState : State {

    private LightCube[] _TargetCubes;

    public override void Enter() {
      base.Enter();
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.ShowReadySlide(LocalizationKeys.kCodeBreakerTextHowToPlayLong,
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