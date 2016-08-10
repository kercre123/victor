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

        private DroneModeTransitionAnimator _RobotAnimator;

        public override void Enter() {
          _LastMessageSentTimestamp = Time.time;

          _DroneModeGame = _StateMachine.GetGame() as DroneModeGame;

          GameObject slide = _DroneModeGame.SharedMinigameView.ShowFullScreenGameStateSlide(
            _DroneModeGame.DroneModeViewPrefab.gameObject, "drone_mode_view_slide");
          _DroneModeControlsSlide = slide.GetComponent<DroneModeControlsSlide>();
          _DroneModeControlsSlide.InitializeCameraFeed(_CurrentRobot);
          EnableInput();

          // TODO Use different colors for bottom and top colors
          // Top = R36, G220, B230
          // Bottom = R25, G131, B213
          // See ticket https://ankiinc.atlassian.net/browse/COZMO-2516 and BackgroundColorController.cs
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

          // If targets are both zero, enable reactionary behavior
          if (_TargetTurnDirection.IsNear(0f, float.Epsilon) && _TargetDriveSpeed_mmps.IsNear(0f, float.Epsilon)
              && _TargetDriveHeadSpeed_radps.IsNear(0f, float.Epsilon)) {
            EnableIdleReactionaryBehaviors(true);
          }
          else {
            EnableIdleReactionaryBehaviors(false);
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
            _CurrentRobot.DriveArc(0f, 0);
            _CurrentRobot.DriveWheels(0f, 0f);
            _DroneModeControlsSlide.TiltText.text = "Drive Arc: \nspeed mmps = " + 0 + " \nradius mm = N/A";
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
          // Drive slower while turning so that we're not spinning like crazy
          float absTurnFactor = Mathf.Abs(turnDirection);
          float driveRobotSpeed_mmps = driveSpeed_mmps * (1 - (absTurnFactor * absTurnFactor * absTurnFactor)); // Cubic ease

          // Turn more sharply with a greater tilt
          float turnFactor = (1 - Mathf.Abs(turnDirection));
          float arcRadius_mm = Mathf.Max(_kRadiusMax_mm * turnFactor, _kRadiusMin_mm);
          if (turnDirection > 0f) {
            arcRadius_mm *= -1;
          }

          // TODO remove debug text field
          _DroneModeControlsSlide.TiltText.text = "Drive Arc: \nspeed mmps = " + driveRobotSpeed_mmps + " \nradius mm = " + arcRadius_mm + "\ntilt = " + _TargetTurnDirection;
          _CurrentRobot.DriveArc(driveRobotSpeed_mmps, (int)arcRadius_mm);
          return driveRobotSpeed_mmps;
        }

        private float PointTurnRobotWheels(float turnDirection) {
          float pointTurnSpeed_mmps = _DroneModeGame.PointTurnSpeed_mmps * Mathf.Abs(turnDirection);
          float arcRadius_mm = _kRadiusMin_mm;
          if (turnDirection > 0f) {
            arcRadius_mm *= -1;
          }

          // TODO remove debug text field
          _DroneModeControlsSlide.TiltText.text = "Drive Arc: \nspeed mmps = " + pointTurnSpeed_mmps + " \nradius mm = " + arcRadius_mm + "\ntilt = " + _TargetTurnDirection;
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