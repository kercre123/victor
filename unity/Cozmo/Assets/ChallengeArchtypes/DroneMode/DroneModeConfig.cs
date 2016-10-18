using UnityEngine;
namespace Cozmo {
  namespace Minigame {
    public class DroneModeConfig : MinigameConfigBase {
      public override int NumCubesRequired() {
        return 0;
      }

      public override int NumPlayersRequired() {
        return 1;
      }

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM), Tooltip("(mm/sec) Max negative velocity of Cozmo's movement when slider is in 'Reverse' area. (When slider is fully extended.)")]
      private float _MaxReverseSpeed_mmps = 90f;
      public float MaxReverseSpeed_mmps { get { return _MaxReverseSpeed_mmps; } }

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM), Tooltip("(mm/sec) Max velocity of Cozmo's movement when slider is in 'Forward' area. (When slider is fully extended.)")]
      private float _MaxForwardSpeed_mmps = 120f;
      public float MaxForwardSpeed_mmps { get { return _MaxForwardSpeed_mmps; } }

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM), Tooltip("(rad/sec) Angular velocity of Cozmo's movement when device tilt is non-zero but speed is zero.")]
      private float _PointTurnSpeed_mmps = 6f;
      public float PointTurnSpeed_mmps { get { return _PointTurnSpeed_mmps; } }

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM), Tooltip("(mm/sec) Velocity of Cozmo's movement when slider is in 'Turbo' area.")]
      private float _TurboSpeed_mmps = 160f;
      public float TurboSpeed_mmps { get { return _TurboSpeed_mmps; } }

      [SerializeField, Range(0f, 90f), Tooltip("Degrees size near neutral position that counts as '0' for the purposes of turning.")]
      private float _NeutralTiltSizeDegrees = 10f;
      public float NeutralTiltSizeDegrees { get { return _NeutralTiltSizeDegrees; } }

      [SerializeField, Range(0f, 90f), Tooltip("Degrees size that counts as '1' for the purposes of turning.")]
      private float _MaxTiltSizeDegrees = 50f;
      public float MaxTiltSizeDegrees { get { return _MaxTiltSizeDegrees; } }

      [SerializeField, Tooltip("(mm) Radius of largest circle that Cozmo will drive around when turning. (Tilt of device is very small; < NeutralTiltSizeDegrees).")]
      private float _MaxTurnArcRadius_mm = 255f;
      public float MaxTurnArcRadius_mm { get { return _MaxTurnArcRadius_mm; } }

      [SerializeField, Tooltip("(mm) Radius of smallest circle that Cozmo will drive around when turning at Turbo speed. (Tilt of device is very large; > MaxTiltSizeDegrees)")]
      private float _MinTurnArcRadiusAtMaxSpeed_mm = 60f;
      public float MinTurnArcRadiusAtMaxSpeed_mm { get { return _MinTurnArcRadiusAtMaxSpeed_mm; } }

      [SerializeField, Tooltip("(mm) Radius of smallest circle that Cozmo will drive around when turning. (Tilt of device is very large; > MaxTiltSizeDegrees).")]
      private float _MinTurnArcRadius_mm = 5f;
      public float MinTurnArcRadius_mm { get { return _MinTurnArcRadius_mm; } }

      [SerializeField, Range(0f, 1f), Tooltip("Normalized where 0 is not tilted (< NeutralTiltSizeDegrees) and 1 is fully tilted (> MaxTiltSizeDegrees). Threshold where cozmo will start slowing down for turns if slider speed is up very high.")]
      private float _SlowDownForTurnThreshold = 0.5f;
      public float SlowDownForTurnThreshold { get { return _SlowDownForTurnThreshold; } }

      [SerializeField, Range(0f, 1000f), Tooltip("(rad/s) Turning speed of head.")]
      private float _HeadTurnSpeed_radPerSec = 15f;
      public float HeadTurnSpeed_radPerSec { get { return _HeadTurnSpeed_radPerSec; } }

      [SerializeField, Range(0f, 10000f), Tooltip("(rad/s^2) Accel speed of head.")]
      private float _HeadTurnAccel_radPerSec2 = 20f;
      public float HeadTurnAccel_radPerSec2 { get { return _HeadTurnAccel_radPerSec2; } }

      [SerializeField, Range(0f, 1000f), Tooltip("(rad/s) Turning speed of lift.")]
      private float _LiftTurnSpeed_radPerSec = 10f;
      public float LiftTurnSpeed_radPerSec { get { return _LiftTurnSpeed_radPerSec; } }

      [SerializeField, Range(0f, 10000f), Tooltip("(rad/s^2) Accel speed of lift.")]
      private float _LiftTurnAccel_radPerSec2 = 20f;
      public float LiftTurnAccel_radPerSec2 { get { return _LiftTurnAccel_radPerSec2; } }

      [SerializeField, Range(0f, 1f), Tooltip("(%) Starting lift height, where 0 is minimum and 1 is maximum.")]
      private float _StartingLiftHeight = 0.25f;
      public float StartingLiftHeight { get { return _StartingLiftHeight; } }
    }
  }
}