using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceState : State {
    protected CubePounceGame _CubePounceGame;

    public override void Enter() {
      base.Enter();
      _CubePounceGame = (_StateMachine.GetGame() as CubePounceGame);

      _CubePounceGame.UpdateCubeVisibility();
    }

    public override void Update() {
      base.Update();

      _CubePounceGame.UpdateCubeVisibility();
    }
  }
}
