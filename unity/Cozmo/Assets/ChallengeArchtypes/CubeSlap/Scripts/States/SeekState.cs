using UnityEngine;
using System.Collections;

namespace CubeSlap {
  // Wait for the cube to be in position, then wait for a tap before transferring into Lineup State
  public class SeekState : State {

    private CubeSlapGame _CubeSlapGame;
    private float _ConfirmDelay = 1.0f;
    public float _FirstSeenTimestamp = -1;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubeSlapGame);
      _CubeSlapGame.ShowHowToPlaySlide(CubeSlapGame.kSetUp);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.7f);
      _CubeSlapGame.ResetSlapChance();
    }

    // Target cube is marked as Red if not in the right position, and green if in the right position.
    // Keep it in the right position for the full ConfirmDelay and the game will begin.
    public override void Update() {
      base.Update();
      bool isBad = true;
      LightCube target = _CubeSlapGame.GetCurrentTarget();
      if (target != null) {
        // If Cube is in the right position, enter game state.
        if (target.MarkersVisible) {
          float distance = Vector3.Distance(_CurrentRobot.WorldPosition, target.WorldPosition);
          if (distance < CubeSlapGame.kCubePlaceDist) {
            isBad = false;
            target.SetLEDs(Color.green);
            if (_FirstSeenTimestamp == -1) {
              _FirstSeenTimestamp = Time.time;
            }

            if (Time.time - _FirstSeenTimestamp > _ConfirmDelay) {
              _StateMachine.SetNextState(new SlapGameState());
            }
          }
        }
      }
      if (isBad) {
        if (target != null) {
          target.SetLEDs(Color.red);
        }
        ResetSeenTimestamp();
      }
    }

    private void ResetSeenTimestamp() {
      _FirstSeenTimestamp = -1;
    }

    public override void Exit() {
      base.Exit();
    }
  }
}

