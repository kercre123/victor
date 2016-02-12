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
      GetCubesFurthestApart(cubesForGame, out cubeA, out cubeB);
      _CubeMidpoint = GetMidpoint(cubeA, cubeB);
      Vector2 cubeAlignmentVector = cubeA.WorldPosition - cubeB.WorldPosition;
      Vector2 vectorFromBlocks = GetPerpendicularFacingRobot(cubeAlignmentVector);

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
      // Skip this if we're already close
      Vector2 robotPosition = new Vector2(_CurrentRobot.WorldPosition.x, _CurrentRobot.WorldPosition.y);
      if ((targetPosition - robotPosition).magnitude > kDistanceThreshold) {
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
      float targetAngle = targetRotation.eulerAngles.z;
      float currentAngle = _CurrentRobot.Rotation.eulerAngles.z;
      if (currentAngle < targetAngle + kAngleTolerance && currentAngle > targetAngle - kAngleTolerance) {
        HandleGotoRotationComplete(true);
      }
      else {
        float adjustedTargetAngle = targetAngle;
        float adjustedCurrentAngle = currentAngle;
        if (adjustedTargetAngle < kAngleTolerance) {
          adjustedTargetAngle += 360;
        }
        if (adjustedCurrentAngle < kAngleTolerance) {
          adjustedCurrentAngle += 360;
        }
        if (adjustedCurrentAngle < adjustedTargetAngle + kAngleTolerance && adjustedCurrentAngle > adjustedTargetAngle - kAngleTolerance) {
          HandleGotoRotationComplete(true);
        }
        else {
          _CurrentRobot.GotoPose(_TargetPosition, targetRotation, callback: HandleGotoRotationComplete);
        }
      }
    }

    private void HandleGotoRotationComplete(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(_FirstPlayer));
    }

    #region Target Position calculations

    private void GetCubesFurthestApart(List<LightCube> cubesToCompare, out LightCube cubeA, out LightCube cubeB) {
      cubeA = null;
      cubeB = null;
      float longestDistanceSquared = 0;
      float distanceSquared = 0;
      Vector3 distanceVector;
      for (int rootCube = 0; rootCube < cubesToCompare.Count - 1; rootCube++) {
        for (int otherCube = 1; otherCube < cubesToCompare.Count; otherCube++) {
          distanceVector = cubesToCompare[rootCube].WorldPosition - cubesToCompare[otherCube].WorldPosition;
          distanceSquared = distanceVector.sqrMagnitude;
          if (distanceSquared > longestDistanceSquared) {
            longestDistanceSquared = distanceSquared;
            cubeA = cubesToCompare[rootCube];
            cubeB = cubesToCompare[otherCube];
          }
        }
      }
    }

    private Vector2 GetMidpoint(LightCube cubeA, LightCube cubeB) {
      return (cubeA.WorldPosition + cubeB.WorldPosition) * 0.5f;
    }

    private Vector2 GetPerpendicularFacingRobot(Vector2 vectorToUse) {
      // Use the dot product to figure out how to get the vector facing
      // towards Cozmo perpendicular to the line of blocks
      Vector2 perpendicular = new Vector2(vectorToUse.y, -vectorToUse.x);
      float dotProduct = Vector2.Dot(_CurrentRobot.Right, vectorToUse);
      if (dotProduct < 0) {
        perpendicular *= -1;
      }
      return perpendicular;
    }

    #endregion
  }
}

