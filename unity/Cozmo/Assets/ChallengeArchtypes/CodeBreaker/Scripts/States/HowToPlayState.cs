using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CodeBreaker {
  public class HowToPlayState : State {

    private LightCube[] _TargetCubes;

    public override void Enter() {
      base.Enter();
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      game.ShowHowToPlaySlide(HandleReadyButtonClicked);

      // Play an idle animation on Cozmo; will be inturrupted by 
      // other animations.
      _CurrentRobot.SetIdleAnimation("_LIVE_​");
      _CurrentRobot.TurnOffAllLights();
      List<LightCube> targetCubes = new List<LightCube>();
      foreach (ObservedObject obj in _CurrentRobot.VisibleObjects) {
        if (obj is LightCube) {
          targetCubes.Add(obj as LightCube);

          if (targetCubes.Count >= game.NumCubesInCode) {
            break;
          }
        }
      }
      _TargetCubes = targetCubes.ToArray();
    }

    public void HandleReadyButtonClicked() {
      _StateMachine.SetNextState(new PickCodeState(_TargetCubes));
    }
  }
}