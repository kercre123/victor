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
  public const float kMaxLiftHeightMM = 92f;
  public const float kMinLiftHeightMM = 32f;
  public const float kLiftRequestTime = 3f;
  public const float kMinHeadAngle = -25f;
  public const float kMaxHeadAngle = 45f;
  public const float kHeadHeightMM = 49f;
  public const float kMaxSpeedRadPerSec = 5f;
  public const float kHeadAngleRequestTime = 3f;
  public const float kCarriedObjectHeight = 75f;
  public const float kMaxVoltage = 5f;
  public const float kOriginToLowLiftDDistMM = 28f;

  // Setting this to -0.8 allows the block to be close enough where he
  // can lift it. -0.9 is too far down can can cut off the top of the cube.
  // (great without lift though)
  public const float kIdealBlockViewHeadValue = -0.7f;
  public const float kIdealBlockViewHeadValueWithoutLift = -0.9f;
  public const float kIdealFaceViewHeadValue = 0.5f;
}
