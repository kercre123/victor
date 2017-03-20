using UnityEngine;
using System.Collections.Generic;

namespace MemoryMatch {
  public class MemoryMatchPlayerGoalCozmoPositionReset : PlayerGoalBase {
    public const float kTargetDistance = 125f;
    private MemoryMatchGame _GameInstance;
    private Vector2 _TargetPosition;
    private Quaternion _TargetRotation;
    private Vector2 _CubeMidpoint;
    private float _DistanceThreshold;
    private float _AngleTol_Deg;

    // During a solo mode game we might want cozmo to readjust but not bother the player
    public MemoryMatchPlayerGoalCozmoPositionReset(MemoryMatchGame game, System.Action<PlayerInfo, int> CompleteCallback, float distanceThreshold = 40f, float angleTol_Deg = 20.0f) {
      _GameInstance = game;
      _DistanceThreshold = distanceThreshold;
      _AngleTol_Deg = angleTol_Deg;
      OnPlayerGoalCompleted = CompleteCallback;
    }

    public override void Enter() {
      base.Enter();

      IRobot robot = _GameInstance.CurrentRobot;
      List<LightCube> cubesForGame = new List<LightCube>();
      foreach (int id in _GameInstance.CubeIdsForGame) {
        LightCube lightCube = null;
        // if this is null, very likely that the "cube disconnect" message is coming up.
        if (robot.LightCubes.TryGetValue(id, out lightCube)) {
          cubesForGame.Add(lightCube);
        }
      }
      LightCube cubeA, cubeB;
      LightCube.TryFindCubesFurthestApart(cubesForGame, out cubeA, out cubeB);
      if (cubeA != null && cubeB != null &&
          cubeA.CurrentPoseState != ObservableObject.PoseState.Invalid &&
          cubeB.CurrentPoseState != ObservableObject.PoseState.Invalid) {
        _CubeMidpoint = VectorUtil.Midpoint(cubeA.WorldPosition.xy(), cubeB.WorldPosition.xy());
        Vector2 cubeAlignmentVector = cubeA.WorldPosition - cubeB.WorldPosition;
        Vector2 perpendicularToCubes = cubeAlignmentVector.PerpendicularAlignedWith(_CurrentRobot.Forward.xy());

        // Add the vector to the center of the blocks to figure out the target world position
        _TargetPosition = _CubeMidpoint + (-perpendicularToCubes.normalized * kTargetDistance);
        float targetAngle = Mathf.Atan2(perpendicularToCubes.y, perpendicularToCubes.x) * Mathf.Rad2Deg;
        _TargetRotation = Quaternion.Euler(0, 0, targetAngle);

        MoveToTargetLocation(_TargetPosition, _TargetRotation);
      }
      else {
        HandleGotoRotationComplete(false);
      }
    }

    private void MoveToTargetLocation(Vector2 targetPosition, Quaternion targetRotation) {
      // Skip moving if we're already close to the target
      if (_CurrentRobot != null) {
        if (_CurrentRobot.WorldPosition.xy().IsNear(targetPosition, _DistanceThreshold)) {
          HandleGotoPoseComplete(true);
        }
        else {
          _CurrentRobot.GotoPose(targetPosition, targetRotation, callback: HandleGotoPoseComplete);
        }
      }
      else {
        HandleGotoRotationComplete(false);
      }
    }

    private void HandleGotoPoseComplete(bool success) {
      MoveToTargetRotation(_TargetRotation);
    }

    private void MoveToTargetRotation(Quaternion targetRotation) {
      if (_CurrentRobot != null) {
        if (_CurrentRobot.Rotation.IsSimilarAngle(targetRotation, _AngleTol_Deg)) {
          HandleGotoRotationComplete(true);
        }
        else {
          _CurrentRobot.GotoPose(_TargetPosition, targetRotation, callback: HandleGotoRotationComplete);
        }
      }
      else {
        HandleGotoRotationComplete(false);
      }
    }

    private void HandleGotoRotationComplete(bool success) {
      // Not much to do even if failed.
      CompletePlayerGoal(SUCCESS);
    }

    public override void Exit() {
      base.Exit();
    }

  }
}