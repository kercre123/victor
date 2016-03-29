using UnityEngine;
using System.Collections;

namespace CubePounce {
  // Wait for the cube to be in position, then wait for a tap before transferring into Lineup State
  public class SeekState : State {

    private CubePounceGame _CubeSlapGame;
    private float _ConfirmDelay = 0.5f;
    public float _FirstSeenTimestamp = -1;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubePounceGame);
      _CubeSlapGame.GetCurrentTarget();
      _CubeSlapGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderSetupText);
      _CubeSlapGame.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoSetupText);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _CurrentRobot.SetLiftHeight(0.7f);
      _CubeSlapGame.ResetPounceChance();
      _CubeSlapGame.UpdateScoreboard();
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
          if (distance < CubePounceGame.kCubePlaceDist) {
            isBad = false;
            target.SetLEDs(Color.green);
            if (_FirstSeenTimestamp == -1) {
              _FirstSeenTimestamp = Time.time;
            }

            if (Time.time - _FirstSeenTimestamp > _ConfirmDelay) {
              _StateMachine.SetNextState(new PounceState());
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
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }
}

