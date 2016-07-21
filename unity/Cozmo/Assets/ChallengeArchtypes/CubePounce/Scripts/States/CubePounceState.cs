using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceState : State {
    protected CubePounceGame _CubePounceGame;

    public override void Enter() {
      base.Enter();
      _CubePounceGame = (_StateMachine.GetGame() as CubePounceGame);
      LightCube.OnMovedAction += HandleCubeMoved;
    }

    public override void Exit() {
      base.Exit();
      LightCube.OnMovedAction -= HandleCubeMoved;
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if (id == _CubePounceGame.GetCubeTarget().ID) {
        HandleOurCubeMoved(accX, accY, aaZ);
      }
    }

    protected virtual void HandleOurCubeMoved(float accX, float accY, float aaZ) {
      // Left up to CubePounceStates to respond to if they want
    }
  }
}
