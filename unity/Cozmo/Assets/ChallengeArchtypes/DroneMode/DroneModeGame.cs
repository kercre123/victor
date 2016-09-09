using UnityEngine;
using System.Collections;
using Cozmo.MinigameWidgets;

namespace Cozmo {
  namespace Minigame {
    namespace DroneMode {
      public class DroneModeGame : GameBase {
        private const float _kCalculateSteeringInputInterval_ms = 100f;
        private const float _kChangedTurnDirectionThreshold = 0.0001f;

        public delegate void TurnDirectionChangedHandler(float newNormalizedPitch);
        public event System.Action<float> OnTurnDirectionChanged;

        [SerializeField]
        private DroneModeControlsSlide _DroneModeViewPrefab;
        public DroneModeControlsSlide DroneModeViewPrefab { get { return _DroneModeViewPrefab; } }

        [SerializeField]
        private GameObject _DroneModeHowToPlaySlidePrefab;
        public GameObject DroneModeHowToPlaySlidePrefab { get { return _DroneModeHowToPlaySlidePrefab; } }

        private DroneModeConfig _DroneModeConfig;
        public DroneModeConfig DroneModeConfigData { get { return _DroneModeConfig; } }

        private float _CurrentTurnDirection;
        private IEnumerator _SteeringInputCoroutine;

        protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
          _DroneModeConfig = minigameConfigData as DroneModeConfig;
          InitializeRobot();
          InitializeStateMachine();
        }

        protected override void AddDisabledReactionaryBehaviors() {
          _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToCubeMoved);
          _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToCliff);

          // for some reason this became a reactionary behavior but isn't named like one...
          _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.KnockOverCubes);
          // this one too!
          _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.CantHandleTallStack);
        }

        private void InitializeStateMachine() {
          DroneModeShowInstructionsState instructionState = new DroneModeShowInstructionsState();
          _StateMachine.SetNextState(instructionState);
        }

        private void InitializeRobot() {
          CurrentRobot.VisionWhileMoving(true);
          CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
          CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
          CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
        }

        protected override void SetupViewAfterCozmoReady(SharedMinigameView newView, ChallengeData data) {
          // TODO
        }
        protected override void CleanUpOnDestroy() {
          // TODO
          StopAllCoroutines();
        }

        public void EnableTiltInput() {
          if (_SteeringInputCoroutine == null) {
            _SteeringInputCoroutine = SendSteeringInput();
            StartCoroutine(_SteeringInputCoroutine);
          }
        }

        public void DisableTiltInput() {
          if (_SteeringInputCoroutine != null) {
            StopCoroutine(_SteeringInputCoroutine);
            _SteeringInputCoroutine = null;
          }
        }

        private IEnumerator SendSteeringInput() {
          while (true) {
            float newDevicePitch = (float)Mathf.Atan(Input.acceleration.x / Mathf.Sqrt(Mathf.Pow(Input.acceleration.y, 2) + Mathf.Pow(Input.acceleration.z, 2)));
            float normalizedTurnDirection = MapDevicePitchToTurnDirection(newDevicePitch);

            if (float.IsNaN(normalizedTurnDirection)) {
              normalizedTurnDirection = _CurrentTurnDirection;
            }

            if (!float.IsNaN(normalizedTurnDirection)) {
              _CurrentTurnDirection = normalizedTurnDirection;
              if (OnTurnDirectionChanged != null) {
                OnTurnDirectionChanged(_CurrentTurnDirection);
              }
            }
            yield return new WaitForSeconds(_kCalculateSteeringInputInterval_ms / 1000.0f);
          }
        }

        private float MapDevicePitchToTurnDirection(float newDevicePitch) {
          float normalizedPitch = 0f;
          float halfPi = Mathf.PI * 0.5f;
          newDevicePitch = Mathf.Clamp(newDevicePitch, -halfPi, halfPi);

          if (!newDevicePitch.IsNear(0f, float.Epsilon)) {
            float absDevicePitch = Mathf.Abs(newDevicePitch);
            float neutralPitchRad = Mathf.Deg2Rad * DroneModeConfigData.NeutralTiltSizeDegrees;
            float maxPitchRad = Mathf.Deg2Rad * DroneModeConfigData.MaxTiltSizeDegrees;
            if (absDevicePitch > neutralPitchRad && absDevicePitch <= maxPitchRad) {
              float currentPitch = absDevicePitch - neutralPitchRad;
              float totalRadRange = maxPitchRad - neutralPitchRad;
              normalizedPitch = currentPitch / totalRadRange;
              if (newDevicePitch < 0) {
                normalizedPitch *= -1f;
              }
            }
            else if (absDevicePitch > maxPitchRad) {
              normalizedPitch = 1f;
              if (newDevicePitch < 0) {
                normalizedPitch *= -1f;
              }
            }
          }

          return normalizedPitch;
        }

        public float CalculateDriveWheelSpeed(DroneModeControlsSlide.SpeedSliderSegment sliderSegment, float sliderSegmentValue) {
          float driveWheelSpeed_mmps = 0f;
          switch (sliderSegment) {
          case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
            driveWheelSpeed_mmps = DroneModeConfigData.TurboSpeed_mmps;
            break;
          case DroneModeControlsSlide.SpeedSliderSegment.Forward:
            driveWheelSpeed_mmps = Mathf.Lerp(DroneModeConfigData.PointTurnSpeed_mmps + 1f, DroneModeConfigData.MaxForwardSpeed_mmps, sliderSegmentValue);
            break;
          case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
            driveWheelSpeed_mmps = Mathf.Lerp(DroneModeConfigData.PointTurnSpeed_mmps + 1f, DroneModeConfigData.MaxReverseSpeed_mmps, sliderSegmentValue) * -1;
            break;
          default:
            driveWheelSpeed_mmps = 0f;
            break;
          }
          return driveWheelSpeed_mmps;
        }

        public float CalculateDriveHeadSpeed(DroneModeControlsSlide.HeadSliderSegment sliderSegment, float sliderSegmentValue) {
          float driveHeadSpeed_radps = 0f;
          switch (sliderSegment) {
          case DroneModeControlsSlide.HeadSliderSegment.Forward:
            driveHeadSpeed_radps = DroneModeConfigData.HeadMovementSpeed_radps * sliderSegmentValue;
            break;
          case DroneModeControlsSlide.HeadSliderSegment.Reverse:
            driveHeadSpeed_radps = DroneModeConfigData.HeadMovementSpeed_radps * sliderSegmentValue * -1;
            break;
          default:
            driveHeadSpeed_radps = 0f;
            break;
          }
          return driveHeadSpeed_radps;
        }
      }
    }
  }
}