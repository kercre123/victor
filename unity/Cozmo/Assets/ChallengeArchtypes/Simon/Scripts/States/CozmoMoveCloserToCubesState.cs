using System;
using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class CozmoMoveCloserToCubesState : State {
    public const float kTargetDistance = 125f;
    public const float kDistanceThreshold = 20f;
    float kAngleTolerance = 2.5f;

    private PlayerType _FirstPlayer;

    private Vector2 _TargetPosition;
    private Quaternion _TargetRotation;
    private Vector2 _CubeMidpoint;

    public CozmoMoveCloserToCubesState(PlayerType firstPlayer) {
      _FirstPlayer = firstPlayer;
    }

    public override void Enter() {
      base.Enter();
      List<LightCube> cubesForGame = _StateMachine.GetGame().CubesForGame;
      LightCube cubeA, cubeB;
      CozmoUtil.TryFindCubesFurthestApart(cubesForGame, out cubeA, out cubeB);
      _CubeMidpoint = CozmoUtil.CalculateMidpoint(cubeA, cubeB);
      Vector2 cubeAlignmentVector = cubeA.WorldPosition - cubeB.WorldPosition;
      Vector2 vectorFromBlocks = CozmoUtil.CalculatePerpendicularTowardsRobot(_CurrentRobot, cubeAlignmentVector);

      // Add the vector to the center of the blocks to figure out the target world position
      _TargetPosition = _CubeMidpoint + (vectorFromBlocks.normalized * kTargetDistance);
      float targetAngle = Mathf.Atan2(-vectorFromBlocks.y, -vectorFromBlocks.x);
      _TargetRotation = Quaternion.Euler(0, 0, targetAngle);

      MoveToTargetLocation(_TargetPosition, _TargetRotation);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void MoveToTargetLocation(Vector2 targetPosition, Quaternion targetRotation) {
      // Skip moving if we're already close to the target
      if (CozmoUtil.IsRobotNearPosition(_CurrentRobot, targetPosition, kDistanceThreshold)) {
        _CurrentRobot.GotoPose(targetPosition, targetRotation, callback: HandleGotoPoseComplete);
      }
      else {
        HandleGotoPoseComplete(true);
      }
    }

    private void HandleGotoPoseComplete(bool success) {
      if (success) {
        MoveToTargetRotation(_TargetRotation);
      }
      else {
        MoveToTargetLocation(_TargetPosition, _TargetRotation);
      }
    }

    private void MoveToTargetRotation(Quaternion targetRotation) {
      if (CozmoUtil.IsRobotFacingAngle(_CurrentRobot, targetRotation, kAngleTolerance)) {
        HandleGotoRotationComplete(true);
      }
      else {
        _CurrentRobot.GotoPose(_TargetPosition, targetRotation, callback: HandleGotoRotationComplete);
      }
    }

    private void HandleGotoRotationComplete(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(_FirstPlayer));
    }

    #region Target Position calculations

    #endregion
  }
}

