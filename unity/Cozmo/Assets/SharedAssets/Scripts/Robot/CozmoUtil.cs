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

  public const float kMaxWheelSpeedMM = 220f;
  public const float kMinWheelSpeedMM = 10f;

  public const float kBlockLengthMM = 44f;

  public const float kLocalBusyTime = 1f;

  public const float kMaxLiftHeightMM = 92f;
  public const float kMinLiftHeightMM = 32f;
  public const float kMoveLiftSpeed_radPerSec = 10f;
  public const float kMoveLiftAccel_radPerSec2 = 20f;
  public const float kLiftRequestTime = 3f;

  public const float kMinHeadAngle = -25f;
  public const float kMaxHeadAngle = 45f;
  public const float kHeadHeightMM = 49f;
  public const float kMoveHeadSpeed_radPerSec = 15f;
  public const float kMoveHeadAccel_radPerSec2 = 20f;
  public const float kHeadAngleRequestTime = 3f;

  public const float kCarriedObjectHeight = 75f;

  public const float kMaxVoltage = 5f;

  public const float kOriginToLowLiftDDistMM = 28f;

  // Setting this to -0.8 allows the block to be close enough where he
  // can lift it. -0.9 is too far down can can cut off the top of the cube.
  // (great without lift though)
  public const float kIdealBlockViewHeadValue = -0.65f;
  public const float kIdealBlockViewHeadValueWithoutLift = -0.9f;

  // It's hard to look too high for seeing/enrolling a face, so bias way up
  public const float kIdealFaceViewHeadValue = 0.95f;

  /// <summary>
  /// Checks whether the edge of the observed object is at the desired distance to the input position, within specified tolerance 
  /// </summary>
  public static bool ObjectEdgeWithinXYTolerance(Vector3 basePosition, ActiveObject obsObject, float desiredDistance_mm, float distanceTolerance_mm) {
    // Update the desired distance to include the half size of the object. Using the X axis size because for now our objects are symmetrical
    float updatedDesiredDist_mm = desiredDistance_mm + (obsObject.Size.x * 0.5f);
    float desiredMax_mm = (updatedDesiredDist_mm + distanceTolerance_mm);
    float desiredMaxSqr_mm = desiredMax_mm * desiredMax_mm;
    float desiredMin_mm = (updatedDesiredDist_mm - distanceTolerance_mm);
    float desiredMinSqr_mm = desiredMin_mm * desiredMin_mm;

    Vector3 posDifference = obsObject.WorldPosition - basePosition;
    posDifference.z = 0.0f;
    float actualSqr_mm = posDifference.sqrMagnitude;

    return (actualSqr_mm > desiredMinSqr_mm && actualSqr_mm < desiredMaxSqr_mm);
  }

  public static bool ObjectEdgeWithinXYDistance(Vector3 basePosition, ActiveObject obsObject, float desiredDistance_mm) {
    float dummyOut;
    return ObjectEdgeWithinXYDistance(basePosition, obsObject, desiredDistance_mm, out dummyOut);
  }

  public static bool ObjectEdgeWithinXYDistance(Vector3 basePosition, ActiveObject obsObject, float desiredDistance_mm, out float actualDistance_mm_out) {
    float distanceThreshold_mm = desiredDistance_mm + (obsObject.Size.x * 0.5f);
    float distanceThresholdSqr_mm = distanceThreshold_mm * distanceThreshold_mm;

    Vector3 positionDifference = obsObject.WorldPosition - basePosition;
    positionDifference.z = 0f;
    float distanceSqr_mm = positionDifference.sqrMagnitude;

    actualDistance_mm_out = distanceSqr_mm;
    return distanceSqr_mm < distanceThresholdSqr_mm;
  }

  /// <summary>
  /// Checks whether the XY vector from base to target positions is aligned with the specified world base direction, within specified angle
  /// </summary>
  public static bool PointWithinXYAngleTolerance(Vector3 basePosition, Vector3 targetPosition, float baseDirection_deg, float angleTolerance_deg) {
    float actualAngle_deg = Mathf.Rad2Deg * Mathf.Atan2(targetPosition.y - basePosition.y, targetPosition.x - basePosition.x);
    float actualDiff_deg = Mathf.Abs(Mathf.DeltaAngle(baseDirection_deg, actualAngle_deg));
    return (actualDiff_deg <= angleTolerance_deg);
  }


  public static float HeadAngleFactorToRadians(float angleFactor, bool useExactAngle) {

    float radians = angleFactor;

    if (!useExactAngle) {
      if (angleFactor >= 0f) {
        radians = Mathf.Lerp(0f, CozmoUtil.kMaxHeadAngle * Mathf.Deg2Rad, angleFactor);
      }
      else {
        radians = Mathf.Lerp(0f, CozmoUtil.kMinHeadAngle * Mathf.Deg2Rad, -angleFactor);
      }
    }
    return radians;
  }
}
