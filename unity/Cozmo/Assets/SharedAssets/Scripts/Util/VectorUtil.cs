using System;
using System.Collections.Generic;
using UnityEngine;

public static class VectorUtil {
  #region Vector helpers

  public static Vector3 x0y(this Vector2 v) {
    return new Vector3(v.x, 0, v.y);
  }

  public static Vector2 xz(this Vector3 v) {
    return new Vector2(v.x, v.z);
  }

  // clear the z component
  public static Vector3 xy0(this Vector3 v) {
    return new Vector3(v.x, v.y, 0);
  }

  public static Vector2 xy(this Vector3 v) {
    return new Vector2(v.x, v.y);
  }

  public static Vector2 Perpendicular(this Vector2 v) {
    return new Vector2(v.y, -v.x);
  }

  public static Vector2 PerpendicularAlignedWith(this Vector2 v, Vector2 alignVector) {
    // Get a vector perpendicular to v
    Vector2 perpendicular = v.Perpendicular();

    // Use the dot product to determine if the perpendicular is facing away
    // from the the desired alignment, if so then flip the vector.
    float dotProduct = Vector2.Dot(alignVector, perpendicular);
    if (dotProduct < 0) {
      perpendicular *= -1;
    }
    return perpendicular;
  }

  public static bool IsNear(this Vector2 v, Vector2 targetPosition, float distanceTolerance_mm) {
    return (((targetPosition - v).sqrMagnitude) <= (distanceTolerance_mm * distanceTolerance_mm));
  }

  public static bool IsNear(this Vector3 v, Vector3 targetPosition, float distanceTolerance_mm) {
    return (((targetPosition - v).sqrMagnitude) <= (distanceTolerance_mm * distanceTolerance_mm));
  }

  public static Vector2 Average(this IEnumerable<Vector2> vectors) {
    Vector2 sum = Vector2.zero;
    int count = 0;
    foreach (Vector2 v in vectors) {
      sum += v;
      count++;
    }
    Vector2 average = Vector2.zero;
    if (count > 0) {
      average = sum / count;
    }
    return average;
  }

  public static Vector3 Average(this IEnumerable<Vector3> vectors) {
    Vector3 sum = Vector3.zero;
    int count = 0;
    foreach (Vector3 v in vectors) {
      sum += v;
      count++;
    }
    Vector3 average = Vector3.zero;
    if (count > 0) {
      average = sum / count;
    }
    return average;
  }

  public static Vector2 Midpoint(Vector2 v1, Vector2 v2) {
    return (v1 + v2) * 0.5f;
  }

  public static Vector3 Midpoint(Vector3 v1, Vector3 v2) {
    return (v1 + v2) * 0.5f;
  }


  public static bool Approximately(this Vector2 a, Vector2 b) {
    return Mathf.Approximately(a.x, b.x) &&
      Mathf.Approximately(a.y, b.y);
  }

  public static bool Approximately(this Vector3 a, Vector3 b) {
    return Mathf.Approximately(a.x, b.x) &&
      Mathf.Approximately(a.y, b.y) &&
      Mathf.Approximately(a.z, b.z);
  }
  #endregion

  #region Quaternion helpers

  public static Quaternion xRotation(this Quaternion q) {
    return Quaternion.Euler(q.eulerAngles.x, 0, 0);
  }

  public static Quaternion yRotation(this Quaternion q) {
    return Quaternion.Euler(0, q.eulerAngles.y, 0);
  }

  public static Quaternion zRotation(this Quaternion q) {
    return Quaternion.Euler(0, 0, q.eulerAngles.z);
  }

  public static bool IsSimilarAngle(this Quaternion q, Quaternion targetRotation, float angleTolerance_deg) {
    float targetAngle = targetRotation.eulerAngles.z;
    float currentAngle = q.eulerAngles.z;
    return IsSimilarAngle(currentAngle, targetAngle, angleTolerance_deg);
  }

  public static bool IsSimilarAngle(float rotation_deg, float targetRotation_deg, float angleTolerance_deg) {
    float diffAngle = CalculateMinAngleBetween(rotation_deg, targetRotation_deg);
    return diffAngle <= angleTolerance_deg;
  }

  #endregion

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

  public static bool IsNear(this float a, float b, float threshold) {
    return (Math.Abs(a - b) < threshold);
  }

  #endregion
}

