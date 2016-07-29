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

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM)]
      private float _MaxReverseSpeed_mmps = 90f;
      public float MaxReverseSpeed_mmps { get { return _MaxReverseSpeed_mmps; } }

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM)]
      private float _MaxForwardSpeed_mmps = 120f;
      public float MaxForwardSpeed_mmps { get { return _MaxForwardSpeed_mmps; } }

      [SerializeField, Range(0f, 6.28f)]
      private float _PointTurnSpeed_mmps = 15f;
      public float PointTurnSpeed_mmps { get { return _PointTurnSpeed_mmps; } }

      [SerializeField, Range(0f, CozmoUtil.kMaxWheelSpeedMM)]
      private float _TurboSpeed_mmps = 160f;
      public float TurboSpeed_mmps { get { return _TurboSpeed_mmps; } }

      [SerializeField, Range(0f, 3.14f)]
      private float _HeadMovementSpeed_radps = 0.5f;
      public float HeadMovementSpeed_radps { get { return _HeadMovementSpeed_radps; } }

      [SerializeField, Range(0f, 1f)]
      private float _NeutralTiltSize = 0.15f;
      public float NeutralTiltSize { get { return _NeutralTiltSize; } }

      [SerializeField, Range(0f, 1f)]
      private float _StartingLiftHeight = 0.25f;
      public float StartingLiftHeight { get { return _StartingLiftHeight; } }
    }
  }
}