using UnityEngine;
using System.Collections;

namespace CubeSlap {
  // Wait for the cube to be in position, then wait for a tap before transferring into Lineup State
  public class SeekState : State {

    private CubeSlapGame _CubeSlapGame;
    private float _ConfirmDelay = 0.5f;
    public float _FirstSeenTimestamp = -1;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubeSlapGame);
      _CubeSlapGame.GetCurrentTarget();
      _CubeSlapGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderSetupText);
      _CubeSlapGame.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoSetupText);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.7f);
      _CubeSlapGame.ResetSlapChance();
    }

    // Target cube is marked as Red if not in the right position, and green if in the right position.
    // Keep it in the right position for the full ConfirmDelay and the game will begin.
    public override void Update() {
      base.Update();
      bool isBad = true;
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
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
          else if (Vector3.Dot(_CurrentRobot.Forward, (target.WorldPosition - _CurrentRobot.WorldPosition).normalized) > 0.92f) {
            // the robot is lined up but not close enough to drive toward it to start the game.
            _CurrentRobot.DriveWheels(15.0f, 15.0f);
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
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }
}

