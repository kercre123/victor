using UnityEngine;

namespace Cozmo {
  namespace Minigame {
    namespace DroneMode {
      public class DroneModeDriveCozmoState : State {
        private const float _kSendMessageInterval_sec = 0.1f;
        private const float _kDriveSpeedChangeThreshold_mmps = 1f;
        private const float _kTurnDirectionChangeThreshold = 0.01f;
        private const float _kHeadTiltChangeThreshold_radps = 0.01f;
        private const float _kLiftFactorThreshold = 0.01f;

        private const float _kRadiusMax_mm = 255f;
        private const float _kRadiusMin_mm = 2f;

        private DroneModeGame _DroneModeGame;
        private DroneModeControlsSlide _DroneModeControlsSlide;

        private float _CurrentDriveSpeed_mmps;
        private float _TargetDriveSpeed_mmps;
        private float _CurrentTurnDirection;
        private float _TargetTurnDirection;
        private float _CurrentDriveHeadSpeed_radps;
        private float _TargetDriveHeadSpeed_radps;

        private float _LastMessageSentTimestamp;

        private bool _IsDrivingWheels = false;
        private bool _IsDrivingHead = false;

        private float _StoppedDrivingHeadTimestamp;
        private float _LockedHeadAngle_rad = 0f;

        private DroneModeTransitionAnimator _RobotAnimator;

        public string TiltDrivingDebugText = "";
        public string HeadDrivingDebugText = "";

        public override void Enter() {
          _LastMessageSentTimestamp = Time.time;
          _StoppedDrivingHeadTimestamp = -1f;

          _DroneModeGame = _StateMachine.GetGame() as DroneModeGame;

          GameObject slide = _DroneModeGame.SharedMinigameView.ShowFullScreenGameStateSlide(
            _DroneModeGame.DroneModeViewPrefab.gameObject, "drone_mode_view_slide");
          _DroneModeControlsSlide = slide.GetComponent<DroneModeControlsSlide>();
          _DroneModeControlsSlide.InitializeCameraFeed(_CurrentRobot);
          EnableInput();

          UIManager.Instance.BackgroundColorController.SetBackgroundColor(UI.BackgroundColorController.BackgroundColor.TintMe,
                                                                          _DroneModeControlsSlide.BackgroundColor);
          _DroneModeGame.SharedMinigameView.HideMiddleBackground();
          _DroneModeGame.SharedMinigameView.SetQuitButtonGraphic(_DroneModeControlsSlide.QuitButtonSprite);

          // Show how to play when the player plays drone mode for the first time
          int timesPlayedDroneMode = 0;
          DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.TryGetValue(_DroneModeGame.ChallengeID, out timesPlayedDroneMode);
          if (timesPlayedDroneMode <= 0) {
            _DroneModeControlsSlide.OpenHowToPlayView();
          }

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

          _DroneModeControlsSlide.TiltText.text = TiltDrivingDebugText + HeadDrivingDebugText;
          _DroneModeControlsSlide.DebugText.text = _RobotAnimator.ToString();
        }

        public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
          // Don't show the "don't move cozmo" ui
        }

        public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
          SetupRobotForDriveState();
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
          _CurrentRobot.SetLiftHeight(_DroneModeGame.DroneModeConfigData.StartingLiftHeight);
        }

        private void SendDriveRobotMessages() {
          if (Time.time - _LastMessageSentTimestamp > _kSendMessageInterval_sec) {
            _LastMessageSentTimestamp = Time.time;
            bool droveWheels = DriveWheelsIfNeeded();
            bool droveHead = DriveHeadIfNeeded();
            DriveLiftIfNeeded();

            if (_IsDrivingHead != droveHead || _IsDrivingWheels != droveWheels) {
              // If targets are both zero, enable reactionary behavior
              if (!droveHead && !droveWheels) {
                EnableIdleReactionaryBehaviors(true);

                // Also make sure to zero out animations
                _RobotAnimator.PlayTransitionAnimation(DroneModeControlsSlide.SpeedSliderSegment.Neutral);
              }
              else {
                EnableIdleReactionaryBehaviors(false);
              }

              _IsDrivingHead = droveHead;
              _IsDrivingWheels = droveWheels;
            }
          }
        }

        private bool DriveWheelsIfNeeded() {
          bool droveWheels = false;
          if (ShouldPointTurn(_TargetDriveSpeed_mmps, _TargetTurnDirection)) {
            _CurrentDriveSpeed_mmps = PointTurnRobotWheels(_TargetTurnDirection);
            _CurrentTurnDirection = _TargetTurnDirection;
            droveWheels = true;
          }
          else if (ShouldDrive(_TargetDriveSpeed_mmps, _TargetTurnDirection)) {
            _CurrentDriveSpeed_mmps = DriveRobotWheels(_TargetDriveSpeed_mmps, _TargetTurnDirection);
            _CurrentTurnDirection = _TargetTurnDirection;
            droveWheels = true;
          }
          else if (ShouldStopDriving(_TargetDriveSpeed_mmps, _CurrentDriveSpeed_mmps, _TargetTurnDirection)) {
            _TargetDriveSpeed_mmps = 0f;
            _CurrentDriveSpeed_mmps = 0f;
            _TargetTurnDirection = 0f;
            _CurrentTurnDirection = 0f;
            _CurrentRobot.DriveArc(0f, 0);
            _CurrentRobot.DriveWheels(0f, 0f);
            TiltDrivingDebugText = "Drive Arc: \nspeed mmps = " + 0 + " \nradius mm = N/A";
          }
          return droveWheels;
        }

        private bool DriveHeadIfNeeded() {
          bool droveHead = false;
          if (ShouldDriveHead(_TargetDriveHeadSpeed_radps)) {
            HeadDrivingDebugText = "\nPlayer driving head   timestamp=" + _StoppedDrivingHeadTimestamp;
            _CurrentRobot.DriveHead(_TargetDriveHeadSpeed_radps);
            _CurrentDriveHeadSpeed_radps = _TargetDriveHeadSpeed_radps;
            droveHead = true;
            _StoppedDrivingHeadTimestamp = -1f;
          }
          else if (ShouldStopDriveHead(_TargetDriveHeadSpeed_radps)) {
            _TargetDriveSpeed_mmps = 0f;
            _CurrentDriveHeadSpeed_radps = 0f;
            _CurrentRobot.DriveHead(0f);
            droveHead = true;
            _StoppedDrivingHeadTimestamp = Time.time;
            _LockedHeadAngle_rad = _CurrentRobot.HeadAngle;
            _CurrentRobot.SetHeadAngle(_LockedHeadAngle_rad, useExactAngle: true);
            HeadDrivingDebugText = "\nPlayer stop driving head   timestamp=" + _StoppedDrivingHeadTimestamp;
          }
          else if (ShouldHoldHead()) {
            HeadDrivingDebugText = "\nCozmo holding head   timestamp=" + _StoppedDrivingHeadTimestamp;
            _CurrentRobot.SetHeadAngle(_LockedHeadAngle_rad, useExactAngle: true);
            droveHead = true;
          }
          else if (ShouldStopHoldHead()) {
            _StoppedDrivingHeadTimestamp = -1f;
            HeadDrivingDebugText = "\nCozmo stop holding head   timestamp=" + _StoppedDrivingHeadTimestamp;
          }
          return droveHead;
        }

        private void DriveLiftIfNeeded() {
          // Ideally we could do this check after every animation end, but this works for now.
          if (!_CurrentRobot.LiftHeightFactor.IsNear(_DroneModeGame.DroneModeConfigData.StartingLiftHeight, _kLiftFactorThreshold)) {
            _CurrentRobot.SetLiftHeight(_DroneModeGame.DroneModeConfigData.StartingLiftHeight);
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
          return !targetHeadSpeed.IsNear(_CurrentDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps)
                                 && !targetHeadSpeed.IsNear(0f, _kHeadTiltChangeThreshold_radps);
        }

        private bool ShouldStopDriveHead(float targetHeadSpeed) {
          return targetHeadSpeed.IsNear(0f, _kHeadTiltChangeThreshold_radps)
                                && !targetHeadSpeed.IsNear(_CurrentDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps)
                                && _StoppedDrivingHeadTimestamp == -1f;
        }

        private bool ShouldHoldHead() {
          return _StoppedDrivingHeadTimestamp != -1f
                                && ((Time.time - _StoppedDrivingHeadTimestamp) < _DroneModeGame.DroneModeConfigData.HeadIdleDelay_s);
        }

        private bool ShouldStopHoldHead() {
          return _StoppedDrivingHeadTimestamp != -1f
            && ((Time.time - _StoppedDrivingHeadTimestamp) >= _DroneModeGame.DroneModeConfigData.HeadIdleDelay_s);
        }

        private float DriveRobotWheels(float driveSpeed_mmps, float turnDirection) {
          float driveRobotSpeed_mmps = 0f;
          float arcRadius_mm = _kRadiusMax_mm;
          if (driveSpeed_mmps > 0) {
            // Drive slower while turning so that we're not spinning like crazy
            float absTurnFactor = Mathf.Abs(turnDirection);
            driveRobotSpeed_mmps = driveSpeed_mmps * (1 - (absTurnFactor * absTurnFactor * absTurnFactor)); // Cubic ease

            // Turn more sharply with a greater tilt
            float turnFactor = (1 - Mathf.Abs(turnDirection));
            arcRadius_mm = Mathf.Max(_kRadiusMax_mm * turnFactor, _kRadiusMin_mm);
            if (turnDirection > 0f) {
              arcRadius_mm *= -1;
            }
          }
          else {
            // Don't turn when going backwards
            driveRobotSpeed_mmps = driveSpeed_mmps;
          }

          // TODO remove debug text field
          TiltDrivingDebugText = "Drive Arc: \nspeed mmps = " + driveRobotSpeed_mmps + " \nradius mm = " + arcRadius_mm + "\ntilt = " + _TargetTurnDirection;
          _CurrentRobot.DriveArc(driveRobotSpeed_mmps, (int)arcRadius_mm);
          return driveRobotSpeed_mmps;
        }

        private float PointTurnRobotWheels(float turnDirection) {
          float pointTurnSpeed_mmps = _DroneModeGame.DroneModeConfigData.PointTurnSpeed_mmps * Mathf.Abs(turnDirection);
          float arcRadius_mm = _kRadiusMin_mm;
          if (turnDirection > 0f) {
            arcRadius_mm *= -1;
          }

          // TODO remove debug text field
          TiltDrivingDebugText = "Drive Arc: \nspeed mmps = " + pointTurnSpeed_mmps + " \nradius mm = " + arcRadius_mm + "\ntilt = " + _TargetTurnDirection;
          _CurrentRobot.DriveArc(pointTurnSpeed_mmps, (int)arcRadius_mm);
          return pointTurnSpeed_mmps;
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
          }
          else if (newTurnDirection.IsNear(0f, _kTurnDirectionChangeThreshold)) {
            _TargetTurnDirection = 0f;
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

        private void EnableIdleReactionaryBehaviors(bool enable) {
          _CurrentRobot.RequestEnableReactionaryBehavior("drone_mode", Anki.Cozmo.BehaviorType.AcknowledgeFace, enable);
          _CurrentRobot.RequestEnableReactionaryBehavior("drone_mode", Anki.Cozmo.BehaviorType.AcknowledgeObject, enable);
          _CurrentRobot.RequestEnableReactionaryBehavior("drone_mode", Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement, enable);
        }
      }
    }
  }
}