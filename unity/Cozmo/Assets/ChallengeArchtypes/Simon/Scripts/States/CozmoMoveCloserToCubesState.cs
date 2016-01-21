using System;
using UnityEngine;

namespace Simon {
  public class CozmoMoveCloserToCubesState : State {
    public const float kDriveWheelSpeed = 80f;

    public const float kTargetDistance = 200f;

    private Vector3 _TargetPosition;

    private Quaternion _TargetRotation;

    private PlayerType _FirstPlayer;

    public CozmoMoveCloserToCubesState(PlayerType firstPlayer) {
      _FirstPlayer = firstPlayer;
    }

    public override void Enter() {
      base.Enter();
      var sumPos = Vector3.zero;
      foreach (var cube in _CurrentRobot.LightCubes.Values) {
        sumPos += cube.WorldPosition;
      }
      var averagePos = sumPos / _CurrentRobot.LightCubes.Count;

      var currentPos = _CurrentRobot.WorldPosition;

      var delta = (currentPos - averagePos).normalized;

      _TargetPosition = averagePos + kTargetDistance * delta;

      _TargetRotation = Quaternion.LookRotation(delta, Vector3.up);

      _CurrentRobot.GotoPose(_TargetPosition, _TargetRotation, callback: HandleGotoPoseComplete);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void HandleGotoPoseComplete(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(_FirstPlayer));
    }
  }
}

