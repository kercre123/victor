using UnityEngine;
using System;
using System.Collections.Generic;
using System.Reflection;

/// <summary>
///   utility class for convenience methods and constants relevant to cozmo
///   much of these constants should migrate to clad ultimately
/// </summary>
public static class CozmoUtil {
  public const float kSmallScreenMaxHeight = 3f;
  public const float kMaxWheelSpeedMM = 160f;
  public const float kMinWheelSpeedMM = 10f;
  public const float kBlockLengthMM = 44f;
  public const float kLocalBusyTime = 1f;
  public const float kMaxLiftHeightMM = 90f;
  public const float kMinLiftHeightMM = 32f;
  public const float kLiftRequestTime = 3f;
  public const float kMinHeadAngle = -25f;
  public const float kMaxHeadAngle = 34f;
  public const float kHeadHeightMM = 49f;
  public const float kMaxSpeedRadPerSec = 5f;
  public const float kHeadAngleRequestTime = 3f;
  public const float kCarriedObjectHeight = 75f;
  public const float kMaxVoltage = 5f;
  public const float kOriginToLowLiftDDistMM = 28f;

  public const float kIdealBlockViewHeadValue = -0.6f;

  #region Math helpers

  /// <summary>
  /// Calculates the minimum angle between angle1 and angle2, where 
  /// angle1 and angle2 are between 0 inclusive and 360 exclusive
  /// </summary>
  public static float CalculateMinAngleBetween(float angle1, float angle2) {
    float angleDiff = Mathf.Abs(angle1 - angle2);
    if (angleDiff > 180.0f) {
      if (angleDiff <= 360f) {
        angleDiff = 360.0f - angleDiff;
      }
      else {
        DAS.Error("CozmoUtil", string.Format("CalculateMinAngleBetween: angle1 and angle2 are out of range [0,360)!  angle1: {0}  angle2: {1}",
          angle1, angle2));
      }
    }
    return angleDiff;
  }

  public static Vector2 CalculatePerpendicular(Vector2 vector) {
    return new Vector2(vector.y, -vector.x);
  }

  #endregion

  #region LightCube helpers

  public static bool TryFindCubesFurthestApart(List<LightCube> cubesToCompare, out LightCube cubeA, out LightCube cubeB) {
    bool success = false;
    cubeA = null;
    cubeB = null;
    if (cubesToCompare.Count >= 2) {
      float longestDistanceSquared = -1f;
      float distanceSquared = -1f;
      Vector3 distanceVector;
      // Check 0->1, 0->2, 0->3... 0->n then check 1->2, 1->3,...1->n all the way to (n-1)->n
      // Distance checks are communicable so there's no use checking 0->1 and 1->0
      for (int rootCube = 0; rootCube < cubesToCompare.Count - 1; rootCube++) {
        for (int otherCube = rootCube + 1; otherCube < cubesToCompare.Count; otherCube++) {
          distanceVector = cubesToCompare[rootCube].WorldPosition - cubesToCompare[otherCube].WorldPosition;
          distanceSquared = distanceVector.sqrMagnitude;
          if (distanceSquared > longestDistanceSquared) {
            longestDistanceSquared = distanceSquared;
            cubeA = cubesToCompare[rootCube];
            cubeB = cubesToCompare[otherCube];
          }
        }
      }
      success = true;
    }
    else {
      DAS.Error("CozmoUtil", string.Format("GetCubesFurthestApart: cubesToCompare has less than 2 cubes! cubesToCompare.Count: {0}", 
        cubesToCompare.Count));
      if (cubesToCompare.Count == 1) {
        cubeA = cubesToCompare[0];
      }
    }
    return success;
  }

  public static Vector2 CalculateMidpoint(LightCube cubeA, LightCube cubeB) {
    return (cubeA.WorldPosition + cubeB.WorldPosition) * 0.5f;
  }

  #endregion

  #region Robot helpers

  public static Vector2 CalculatePerpendicularTowardsRobot(IRobot robot, Vector2 vectorToUse) {
    // Get a vector perpendicular to vectorToUse
    Vector2 perpendicular = CalculatePerpendicular(vectorToUse);

    // Use the dot product to determine if the perpendicular is facing away
    // from the robot, if so then flip the vector.
    float dotProduct = Vector2.Dot(robot.Forward, perpendicular);
    if (dotProduct > 0) {
      perpendicular *= -1;
    }
    return perpendicular;
  }

  public static bool IsRobotNearPosition(IRobot robot, Vector2 targetPosition, float distanceTolerance_mm) {
    Vector2 robotPosition = new Vector2(robot.WorldPosition.x, robot.WorldPosition.y);
    return (((targetPosition - robotPosition).sqrMagnitude) > (distanceTolerance_mm * distanceTolerance_mm));
  }

  public static bool IsRobotFacingAngle(IRobot robot, Quaternion targetRotation, float angleTolerance_deg) {
    float targetAngle = targetRotation.eulerAngles.z;
    float currentAngle = robot.Rotation.eulerAngles.z;
    float diffAngle = CalculateMinAngleBetween(currentAngle, targetAngle);
    return diffAngle <= angleTolerance_deg;
  }

  #endregion
}
