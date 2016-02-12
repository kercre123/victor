using System;
using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class CozmoMoveCloserToCubesState : State {
    public const float kTargetDistance = 125f;
    public const float kDistanceThreshold = 20f;

    private PlayerType _FirstPlayer;

    private Vector3 _TargetPosition;
    private Quaternion _TargetRotation;
    private Vector3 _CubeMidpoint;

    public CozmoMoveCloserToCubesState(PlayerType firstPlayer) {
      _FirstPlayer = firstPlayer;
    }

    public override void Enter() {
      base.Enter();
      List<LightCube> cubesForGame = _StateMachine.GetGame().CubesForGame;
      LightCube cubeA, cubeB;
      GetCubesFurthestApart(cubesForGame, out cubeA, out cubeB);
      _CubeMidpoint = GetMidpoint(cubeA, cubeB);
      Vector3 cubeAlignmentVector = cubeA.WorldPosition - cubeB.WorldPosition;
      Vector3 vectorFromBlocks = GetPerpendicularFacingRobot(cubeAlignmentVector);

      // Add the vector to the center of the blocks to figure out the target world position
      _TargetPosition = _CubeMidpoint + (vectorFromBlocks.normalized * kTargetDistance);
      _TargetRotation = Quaternion.LookRotation(-vectorFromBlocks, Vector3.up);

      MoveToTargetLocation(_TargetPosition, _TargetRotation);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void MoveToTargetLocation(Vector3 targetPosition, Quaternion targetRotation) {
      // Skip this if we're already close
      if ((targetPosition - _CurrentRobot.WorldPosition).magnitude > kDistanceThreshold) {
        _CurrentRobot.GotoPose(targetPosition, targetRotation, callback: HandleGotoPoseComplete);
      }
      else {
        // TODO: Check rotation?
        HandleGotoPoseComplete(true);
      }
    }

    private void HandleGotoPoseComplete(bool success) {
      if (success) {
        // TODO: Check rotation?
        _StateMachine.SetNextState(new WaitForNextRoundSimonState(_FirstPlayer));
      }
      else {
        MoveToTargetLocation(_TargetPosition, _TargetRotation);
      }
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

    private Vector3 GetMidpoint(LightCube cubeA, LightCube cubeB) {
      return (cubeA.WorldPosition + cubeB.WorldPosition) * 0.5f;
    }

    private Vector3 GetPerpendicularFacingRobot(Vector3 vectorToUse) {
      // Use the dot product to figure out how to get the vector facing
      // towards Cozmo perpendicular to the line of blocks
      Vector3 vectorFromBlocks;
      float dotProduct = Vector3.Dot(_CurrentRobot.Right, vectorToUse);
      if (dotProduct > 0) {
        // Forward is Up in webots
        vectorFromBlocks = Vector3.Cross(vectorToUse, Vector3.forward);
      }
      else {
        vectorFromBlocks = Vector3.Cross(Vector3.forward, vectorToUse);
      }
      return vectorFromBlocks;
    }

    #endregion
  }
}

