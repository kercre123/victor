using UnityEngine;
using System;
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
  public const float kMaxSpeedRadPerSec = 5f;
  public const float kHeadAngleRequestTime = 3f;
  public const float kCarriedObjectHeight = 75f;
  public const float kMaxVoltage = 5f;
}
