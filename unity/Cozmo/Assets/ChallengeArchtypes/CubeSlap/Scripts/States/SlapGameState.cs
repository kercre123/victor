using UnityEngine;
using System.Collections;

namespace CubeSlap {
  public class SlapGameState : State {

    private CubeSlapGame _CubeSlapGame;
    private float _SlapDelay;
    private float _FirstTimestamp = -1;
    public bool _SlapTriggered = false;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubeSlapGame);
      _SlapDelay = _CubeSlapGame.GetSlapDelay();
      _FirstTimestamp = Time.time;
    }

    public override void Update() {
      base.Update();
      if (!_SlapTriggered && Time.time - _FirstTimestamp > _SlapDelay) {
        _CubeSlapGame.AttemptSlap();
        _SlapTriggered = true;
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

