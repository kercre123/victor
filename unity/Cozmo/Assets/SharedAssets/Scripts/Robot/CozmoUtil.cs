using UnityEngine;
using System;
using System.Reflection;

/// <summary>
///   utility class for convenience methods and constants relevant to cozmo
///   much of these constants should migrate to clad ultimately
/// </summary>
public static class CozmoUtil {

  public const float SMALL_SCREEN_MAX_HEIGHT = 3f;
  public const float MAX_WHEEL_SPEED_MM = 160f;
  public const float MIN_WHEEL_SPEED_MM = 10f;
  public const float BLOCK_LENGTH_MM = 44f;
  public const float LOCAL_BUSY_TIME = 1f;
  public const float MAX_LIFT_HEIGHT_MM = 90f;
  public const float MIN_LIFT_HEIGHT_MM = 32f;
  public const float LIFT_REQUEST_TIME = 3f;
  public const float MIN_HEAD_ANGLE = -25f;
  public const float MAX_HEAD_ANGLE = 34f;
  public const float MAX_SPEED_RAD_PER_SEC = 5f;
  public const float HEAD_ANGLE_REQUEST_TIME = 3f;
  public const float CARRIED_OBJECT_HEIGHT = 75f;
  public const float MAX_VOLTAGE = 5f;

  public static Vector3 Vector3UnityToCozmoSpace(Vector3 vector) {
    float forward = vector.z;
    float up = vector.y;

    vector.y = forward;
    vector.z = up;

    return vector;
  }

  public static Vector3 Vector3CozmoToUnitySpace(Vector3 vector) {
    float forward = vector.y;
    float up = vector.z;

    vector.z = forward;
    vector.y = up;

    return vector;
  }

}
