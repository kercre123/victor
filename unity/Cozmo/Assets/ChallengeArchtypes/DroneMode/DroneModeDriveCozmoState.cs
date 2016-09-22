using UnityEngine;

namespace Cozmo {
  namespace Minigame {
    namespace DroneMode {
      public class DroneModeDriveCozmoState : State {
        private const float _kSendMessageInterval_sec = 0.1f;
        private const float _kDriveSpeedChangeThreshold_mmps = 1f;
        private const float _kTurnDirectionChangeThreshold = 0.01f;
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

        private bool _IsDrivingWheels = false;
        private bool _IsDrivingHead = false;

        private float _StoppedDrivingHeadTimestamp;
        private float _LockedHeadAngle_rad = 0f;

        private DroneModeTransitionAnimator _RobotAnimator;

        private bool isStill = false;

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
          if (Time.time - _LastMessageSentTimestamp > _kSendMessageInterval_sec
              || ShouldStopDriving(_TargetDriveSpeed_mmps, _CurrentDriveSpeed_mmps, _TargetTurnDirection)
              || ShouldStopDriveHead(_TargetDriveHeadSpeed_radps)) {
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
          // Is the user telling us to point turn?
          if (IsUserPointTurning(_TargetDriveSpeed_mmps, _TargetTurnDirection)) {
            // Send a new message only if there is a change
            if (!_TargetTurnDirection.IsNear(_CurrentTurnDirection, _kTurnDirectionChangeThreshold) || !_CurrentDriveSpeed_mmps.IsNear(_TargetDriveSpeed_mmps, _kDriveSpeedChangeThreshold_mmps)) {
              _CurrentDriveSpeed_mmps = PointTurnRobotWheels(_TargetTurnDirection);
              _CurrentTurnDirection = _TargetTurnDirection;
            }
            droveWheels = true;
          }
          // Is the user telling us to drive?
          else if (IsUserDriving(_TargetDriveSpeed_mmps, _TargetTurnDirection)) {
            // Send a new message only if there is a change
            if (!_TargetTurnDirection.IsNear(_CurrentTurnDirection, _kTurnDirectionChangeThreshold)
                || !_TargetDriveSpeed_mmps.IsNear(_CurrentDriveSpeed_mmps, _kDriveSpeedChangeThreshold_mmps)) {
              _CurrentDriveSpeed_mmps = DriveRobotWheels(_TargetDriveSpeed_mmps, _TargetTurnDirection);
              _CurrentTurnDirection = _TargetTurnDirection;
            }
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
          // Is the user telling us to drive the head?
          if (IsUserDrivingHead(_TargetDriveHeadSpeed_radps)) {
            // Send a new message only if there is a change
            if (!_TargetDriveHeadSpeed_radps.IsNear(_CurrentDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps)) {
              _CurrentRobot.DriveHead(_TargetDriveHeadSpeed_radps);
              _CurrentDriveHeadSpeed_radps = _TargetDriveHeadSpeed_radps;
            }
            HeadDrivingDebugText = "\nPlayer driving head   timestamp=" + _StoppedDrivingHeadTimestamp;
            _StoppedDrivingHeadTimestamp = -1f;
            droveHead = true;
            if (!isStill) {
              _RobotAnimator.PushHeadStill ();
              isStill = true;
            }
          }
          else if (ShouldStopDriveHead(_TargetDriveHeadSpeed_radps)) {
            _TargetDriveHeadSpeed_radps = 0f;
            _CurrentDriveHeadSpeed_radps = 0f;
            _CurrentRobot.DriveHead(0f);
            _LockedHeadAngle_rad = _CurrentRobot.HeadAngle;
            _CurrentRobot.SetHeadAngle(_LockedHeadAngle_rad, useExactAngle: true);
            _StoppedDrivingHeadTimestamp = Time.time;
            HeadDrivingDebugText = "\nPlayer stop driving head   timestamp=" + _StoppedDrivingHeadTimestamp;
            droveHead = true;
          }
          else if (ShouldStopHoldHead()) {
            _StoppedDrivingHeadTimestamp = -1f;
            HeadDrivingDebugText = "\nCozmo stop holding head   timestamp=" + _StoppedDrivingHeadTimestamp;
            _RobotAnimator.PopHeadStill();
            isStill = false;
          }
          return droveHead;
        }

        private void DriveLiftIfNeeded() {
          // Ideally we could do this check after every animation end, but this works for now.
          if (!_CurrentRobot.LiftHeightFactor.IsNear(_DroneModeGame.DroneModeConfigData.StartingLiftHeight, _kLiftFactorThreshold)) {
            _CurrentRobot.SetLiftHeight(_DroneModeGame.DroneModeConfigData.StartingLiftHeight);
          }
        }

        private bool IsUserPointTurning(float targetDriveSpeed, float targetTurnDirection) {
          return targetDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                 && !targetTurnDirection.IsNear(0f, _kTurnDirectionChangeThreshold);
        }

        private bool IsUserDriving(float targetDriveSpeed, float targetTurnDirection) {
          return !targetDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps);
        }

        private bool ShouldStopDriving(float targetDriveSpeed, float currentDriveSpeed, float targetTurnDirection) {
          return targetDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                 && !currentDriveSpeed.IsNear(0f, _kDriveSpeedChangeThreshold_mmps)
                                 && targetTurnDirection.IsNear(0f, _kTurnDirectionChangeThreshold);
        }

        private bool IsUserDrivingHead(float targetHeadSpeed) {
          return !targetHeadSpeed.IsNear(0f, _kHeadTiltChangeThreshold_radps);
        }

        private bool ShouldStopDriveHead(float targetHeadSpeed) {
          return targetHeadSpeed.IsNear(0f, _kHeadTiltChangeThreshold_radps)
                                && !targetHeadSpeed.IsNear(_CurrentDriveHeadSpeed_radps, _kHeadTiltChangeThreshold_radps)
                                && _StoppedDrivingHeadTimestamp == -1f;
        }

        private bool ShouldStopHoldHead() {
          return _StoppedDrivingHeadTimestamp != -1f
            && ((Time.time - _StoppedDrivingHeadTimestamp) >= _DroneModeGame.DroneModeConfigData.HeadIdleDelay_s);
        }

        private float DriveRobotWheels(float driveSpeed_mmps, float turnDirection) {
          float driveRobotSpeed_mmps = 0f;
          float arcRadius_mm = _DroneModeGame.DroneModeConfigData.MaxTurnArcRadius_mm;

          // Don't turn when going backwards (or when not turning... obviously)
          if (driveSpeed_mmps < 0 || turnDirection.IsNear(0f, _kTurnDirectionChangeThreshold)) {
            driveRobotSpeed_mmps = driveSpeed_mmps;
            _CurrentRobot.DriveWheels(driveSpeed_mmps, driveSpeed_mmps);
          }
          else {
            // COZMO-3737: Drive speed should only be denoted by the slider & not tilt direction
            driveRobotSpeed_mmps = driveSpeed_mmps;

            float normalizedDriveSpeed = Mathf.Abs(driveSpeed_mmps) / _DroneModeGame.DroneModeConfigData.TurboSpeed_mmps;
            float minTurnRadius = Mathf.Lerp(_DroneModeGame.DroneModeConfigData.MinTurnArcRadius_mm,
                                             _DroneModeGame.DroneModeConfigData.MinTurnArcRadiusAtMaxSpeed_mm,
                                             normalizedDriveSpeed);

            // Turn more sharply with a greater tilt
            float absTurnDirection = Mathf.Abs(turnDirection);
            arcRadius_mm = Mathf.Lerp(_DroneModeGame.DroneModeConfigData.MaxTurnArcRadius_mm,
                                      minTurnRadius,
                                      absTurnDirection);
            if (turnDirection > 0f) {
              arcRadius_mm *= -1;
            }
            _CurrentRobot.DriveArc(driveRobotSpeed_mmps, (int)arcRadius_mm);
          }

          TiltDrivingDebugText = "Drive Arc: \nspeed mmps = " + driveRobotSpeed_mmps + " \nradius mm = " + arcRadius_mm + "\ntilt = " + _TargetTurnDirection;
          return driveRobotSpeed_mmps;
        }

        private float PointTurnRobotWheels(float turnDirection) {
          float pointTurnSpeed_mmps = _DroneModeGame.DroneModeConfigData.PointTurnSpeed_mmps * Mathf.Abs(turnDirection);
          float arcRadius_mm = _DroneModeGame.DroneModeConfigData.MinTurnArcRadius_mm;
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