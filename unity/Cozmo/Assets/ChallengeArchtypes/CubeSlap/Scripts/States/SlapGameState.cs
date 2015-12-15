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
      // TODO: Check to see if there's been a change of state to make sure that Cozmo hasn't been tampered with
      // and that players haven't already pulled the cube too early. If they have, return to the Seek state and automatically
      // trigger a failure on the player's part.
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

