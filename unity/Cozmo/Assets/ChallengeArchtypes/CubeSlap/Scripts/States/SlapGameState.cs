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
      _CurrentRobot.SetLiftHeight(1.0f);
      _FirstTimestamp = Time.time;
      _CubeSlapGame.ShowHowToPlaySlide(CubeSlapGame.kWaitForPounce); 
    }

    public override void Update() {
      base.Update();
      // Check to see if there's been a change of state to make sure that Cozmo hasn't been tampered with
      // and that players haven't already pulled the cube too early. If they have, return to the Seek state and automatically
      // trigger a failure on the player's part.
      if (!_SlapTriggered) {
        // If the slap hasn't been triggered, check to make sure the Cube hasn't been tampered with.
        // If the cube is not visible or it has moved outside of the ideal range, trigger a failure.
        LightCube target = _CubeSlapGame.GetCurrentTarget();
        if (target != null) {
          bool didFail = true;
          // Unless the cube is in the right position, trigger a failure since you moved it
          if (target.MarkersVisible) {
            float distance = Vector3.Distance(_CurrentRobot.WorldPosition, target.WorldPosition);
            if (distance < 90.0f) {
              didFail = false;
            }
          }
          if (didFail) {
            _CubeSlapGame.ShowHowToPlaySlide(CubeSlapGame.kCozmoWinEarly);
            _CubeSlapGame.OnFailure();
          }
        }

        if (Time.time - _FirstTimestamp > _SlapDelay) {
          _CubeSlapGame.AttemptSlap();
          _SlapTriggered = true;
        }
      }
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

