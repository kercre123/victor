using UnityEngine;

namespace Cozmo {
  namespace Minigame {
    namespace DroneMode {
      public class DroneModeDriveCozmoState : State {
        private const float _kSendMessageInterval_sec = 0.1f;
        private const float _kDriveSpeedChangeThreshold_mmps = 5f;
        private const float _kTurnDirectionChangeThreshold = 0.05f;
        private const float _kHeadTiltChangeThreshold_radps = 0.05f;
        private const float _kLiftFactorThreshold = 0.05f;

        private DroneModeGame _DroneModeGame;
        private DroneModeControlsSlide _DroneModeControlsSlide;

        private float _CurrentDriveSpeed_mmps;
        private float _TargetDriveSpeed_mmps;
        private float _CurrentTurnDirection;
        private float _TargetTurnDirection;
        private float _CurrentDriveHeadSpeed_radps;
        private float _TargetDriveHeadSpeed_radps;

        private float _LastMessageSentTimestamp;

        private DroneModeTransitionAnimator _RobotAnimator;

        public override void Enter() {
          _LastMessageSentTimestamp = Time.time;

          _DroneModeGame = _StateMachine.GetGame() as DroneModeGame;

          GameObject slide = _DroneModeGame.SharedMinigameView.ShowFullScreenGameStateSlide(
            _DroneModeGame.DroneModeViewPrefab.gameObject, "drone_mode_view_slide");
          _DroneModeControlsSlide = slide.GetComponent<DroneModeControlsSlide>();
          _DroneModeControlsSlide.InitializeCameraFeed(_CurrentRobot);
          EnableInput();

          SetupRobotForDriveState();

          _RobotAnimator = new DroneModeTransitionAnimator(_CurrentRobot);
          _CurrentRobot.EnableDroneMode(true);
        }

        public override void Exit() {
          DisableInput();
          _RobotAnimator.CleanUp();
          _CurrentRobot.EnableDroneMode(false);
        }

        public override void Update() {
          // TODO Check for visible objects and follow if there is no input? 
          // Potentially this will already be handled by reactionary behaviors

          // Send drive wheels / drive head messages if needed
          SendDriveRobotMessages();
        }

        public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
          // Don't show the "don't move cozmo" ui

          // DisableInput();
        }

        public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
          SetupRobotForDriveState();

          // EnableInput();

          // Does this get called? I think Resume might not get called in COZMO-3221
        }

        private void EnableInput() {
          _DroneModeControlsSlide.OnDriveSpeedSegmentValueChanged += HandleDriveSpeedValueChanged;
          _DroneModeControlsSlide.OnDriveSpeedSegmentChanged += HandleDriveSpeedFamilyChanged;
          _DroneModeControlsSlide.OnHeadTiltSegmentValueChanged += HandleHeadTiltValueChanged;

          _DroneModeGame.EnableTiltInput();
          _DroneModeGame.OnTurnDirectionChanged += HandleTurnDirectionChanged;
        }

        private void DisableInput() {
          _DroneModeControlsSlide.OnDriveSpeedSegmentValueChanged -= HandleDriveSpeedValueChanged;
          _DroneModeControlsSlide.OnDriveSpeedSegmentChanged -= HandleDriveSpeedFamilyChanged;
          _DroneModeControlsSlide.OnHeadTiltSegmentValueChanged -= HandleHeadTiltValueChanged;
          _DroneModeGame.DisableTiltInput();
          _DroneModeGame.OnTurnDirectionChanged -= HandleTurnDirectionChanged;
        }

        private void SetupRobotForDriveState() {
          _CurrentRobot.SetLiftHeight(_DroneModeGame.StartingLiftHeight);
        }

        private void SendDriveRobotMessages() {
          if (Time.time - _LastMessageSentTimestamp > _kSendMessageInterval_sec) {
            _LastMessageSentTimestamp = Time.time;
            DriveWheelsIfNeeded();
            DriveHeadIfNeeded();
            DriveLiftIfNeeded();
          }
        }

        private void DriveWheelsIfNeeded() {
          if (ShouldPointTurn(_TargetDriveSpeed_mmps, _TargetTurnDirection)) {
            _CurrentDriveSpeed_mmps = PointTurnRobotWheels(_TargetTurnDirection);
            _CurrentTurnDirection = _TargetTurnDirection;
          }
          else if (ShouldDrive(_TargetDriveSpeed_mmps, _TargetTurnDirection)) {
            _CurrentDriveSpeed_mmps = DriveRobotWheels(_TargetDriveSpeed_mmps, _TargetTurnDirection);
            _CurrentTurnDirection = _TargetTurnDirection;
          }
          else if (ShouldStopDriving(_TargetDriveSpeed_mmps, _CurrentDriveSpeed_mmps, _TargetTurnDirection)) {
            _TargetDriveSpeed_mmps = 0f;
            _CurrentDriveSpeed_mmps = 0f;
            _TargetTurnDirection = 0f;
            _CurrentTurnDirection = 0f;
            _CurrentRobot.DriveWheels(0f, 0f);
          }
        }

        private void DriveHeadIfNeeded() {
          if (ShouldDriveHead(_TargetDriveHeadSpeed_radps)) {
            _CurrentRobot.DriveHead(_TargetDriveHeadSpeed_radps);
            _CurrentDriveHeadSpeed_radps = _TargetDriveHeadSpeed_radps;
          }
          else if (ShouldStopDriveHead(_TargetDriveHeadSpeed_radps)) {
            _TargetDriveSpeed_mmps = 0f;
            _CurrentDriveHeadSpeed_radps = 0f;
            _CurrentRobot.DriveHead(_TargetDriveHeadSpeed_radps);
          }
        }

        private void DriveLiftIfNeeded() {
          // Ideally we could do this check after every animation end, but this works for now.
          if (!_CurrentRobot.LiftHeightFactor.IsNear(_DroneModeGame.StartingLiftHeight, _kLiftFactorThreshold)) {
            _CurrentRobot.SetLiftHeight(_DroneModeGame.StartingLiftHeight);
          }
        }

        private bool ShouldPointTurn(float targetDriveSpeed, float targetTurnDirection) {
          return targetDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                 && !targetTurnDirection.IsNear(0f, _kTurnDirectionChangeThreshold)
                                 && !targetTurnDirection.IsNear(_CurrentTurnDirection, _kTurnDirectionChangeThreshold);
        }

        private bool ShouldDrive(float targetDriveSpeed, float targetTurnDirection) {
          return !targetDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                  && (!targetDriveSpeed.IsNear(_CurrentDriveSpeed_mmps, _kDriveSpeedChangeThreshold_mmps)
                                      || !targetTurnDirection.IsNear(_CurrentTurnDirection, _kTurnDirectionChangeThreshold));
        }

        private bool ShouldStopDriving(float targetDriveSpeed, float currentDriveSpeed, float targetTurnDirection) {
          return targetDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                 && !currentDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                 && targetTurnDirection.IsNear(0f, _kTurnDirectionChangeThreshold);
        }

        private bool ShouldDriveHead(float targetHeadSpeed) {
          return !targetHeadSpeed.IsNear(_CurrentDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps);
        }

        private bool ShouldStopDriveHead(float targetHeadSpeed) {
          return targetHeadSpeed.IsNear(0f, _kHeadTiltChangeThreshold_radps)
                                 && !targetHeadSpeed.IsNear(_CurrentDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps);
        }

        private float DriveRobotWheels(float driveSpeed_mmps, float turnDirection) {
          float leftWheelSpeed_mmps = driveSpeed_mmps;
          float rightWheelSpeed_mmps = driveSpeed_mmps;
          float halfSpeed = driveSpeed_mmps * 0.5f;
          if (turnDirection < 0) {
            leftWheelSpeed_mmps = Mathf.Lerp(-halfSpeed, halfSpeed, 1 - Mathf.Abs(turnDirection));
          }
          else if (turnDirection > 0) {
            rightWheelSpeed_mmps = Mathf.Lerp(-halfSpeed, halfSpeed, 1 - turnDirection);
          }

          _CurrentRobot.DriveWheels(leftWheelSpeed_mmps, rightWheelSpeed_mmps);
          return driveSpeed_mmps;
        }

        private float PointTurnRobotWheels(float turnDirection) {
          float pointTurnSpeed = _DroneModeGame.PointTurnSpeed_mmps * Mathf.Abs(turnDirection);
          float leftWheelSpeed_mmps = pointTurnSpeed;
          float rightWheelSpeed_mmps = pointTurnSpeed;

          // Special case point turns with no speed
          if (turnDirection < 0) {
            leftWheelSpeed_mmps = -pointTurnSpeed;
          }
          else if (turnDirection > 0) {
            rightWheelSpeed_mmps = -pointTurnSpeed;
          }

          _CurrentRobot.DriveWheels(leftWheelSpeed_mmps, rightWheelSpeed_mmps);
          return pointTurnSpeed;
        }

        private void HandleDriveSpeedValueChanged(DroneModeControlsSlide.SpeedSliderSegment newPosition, float newNormalizedValue) {
          float newDriveSpeed_mmps = _DroneModeGame.CalculateDriveWheelSpeed(newPosition, newNormalizedValue);
          if (!newDriveSpeed_mmps.IsNear(_TargetDriveSpeed_mmps, _kDriveSpeedChangeThreshold_mmps)) {
            _TargetDriveSpeed_mmps = newDriveSpeed_mmps;
          }
        }

        private void HandleTurnDirectionChanged(float newTurnDirection) {
          if (!newTurnDirection.IsNear(_TargetTurnDirection, _kTurnDirectionChangeThreshold)) {
            _TargetTurnDirection = newTurnDirection;

            // TODO Remove debug text field
            _DroneModeControlsSlide.TiltText.text = "Tilt: " + _TargetTurnDirection;
          }
        }

        private void HandleHeadTiltValueChanged(DroneModeControlsSlide.HeadSliderSegment newPosition, float newNormalizedValue) {
          float newDriveHeadSpeed_radps = _DroneModeGame.CalculateDriveHeadSpeed(newPosition, newNormalizedValue);
          if (!newDriveHeadSpeed_radps.IsNear(_TargetDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps)) {
            _TargetDriveHeadSpeed_radps = newDriveHeadSpeed_radps;
          }
        }

        private void HandleDriveSpeedFamilyChanged(DroneModeControlsSlide.SpeedSliderSegment currentPosition, DroneModeControlsSlide.SpeedSliderSegment newPosition) {
          _RobotAnimator.PlayTransitionAnimation(newPosition);
        }
      }
    }
  }
}