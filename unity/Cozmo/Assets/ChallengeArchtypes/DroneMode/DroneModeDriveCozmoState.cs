using UnityEngine;

namespace Cozmo {
  namespace Minigame {
    namespace DroneMode {
      public class DroneModeDriveCozmoState : State {
        private const float _kSendMessageInterval_sec = 0.1f;
        private const float _kDriveSpeedChangeThreshold_mmps = 1f;
        private const float _kTurnDirectionChangeThreshold = 0.01f;
        private const float _kHeadFactorThreshold_rad = 0.05f;
        private const float _kLiftFactorThreshold = 0.05f;

        private DroneModeGame _DroneModeGame;
        private DroneModeControlsSlide _DroneModeControlsSlide;

        private float _CurrentDriveSpeed_mmps;
        private float _TargetDriveSpeed_mmps;
        private float _CurrentTurnDirection;
        private float _TargetTurnDirection;

        private float _TargetHeadAngle_rad;

        private float _TargetLiftFactor;

        private float _LastMessageSentTimestamp;

        private bool _IsDrivingWheels = false;
        private bool _IsDrivingHead = false;
        private bool _IsDrivingLift = false;

        private bool _IsPerformingAction = false;

        private DroneModeTransitionAnimator _RobotAnimator;

        public string TiltDrivingDebugText = "";
        public string HeadDrivingDebugText = "";

        public override void Enter() {
          _LastMessageSentTimestamp = Time.time;

          _DroneModeGame = _StateMachine.GetGame() as DroneModeGame;
          _DroneModeGame.OnTurnDirectionChanged += HandleTurnDirectionChanged;

          _TargetLiftFactor = _DroneModeGame.DroneModeConfigData.StartingLiftHeight;
          _CurrentRobot.SetLiftHeight(_TargetLiftFactor);

          GameObject slide = _DroneModeGame.SharedMinigameView.ShowFullScreenGameStateSlide(
            _DroneModeGame.DroneModeViewPrefab.gameObject, "drone_mode_view_slide");
          _DroneModeControlsSlide = slide.GetComponent<DroneModeControlsSlide>();
          _DroneModeControlsSlide.InitializeCameraFeed(_CurrentRobot);
          _DroneModeControlsSlide.InitializeLiftSlider(_TargetLiftFactor);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.LiftCubeButtonData,
                                                     HandleLiftCubeButtonPressed,
                                                     true, // interactableOnlyWhenCubeSeen
                                                     false, // interactableOnlyWhenKnownFaceSeen
                                                     "lift_cube_button",
                                                     DroneModeControlsSlide.ActionContextType.CubeNotInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.RollCubeButtonData,
                                                     HandleRollCubeButtonPressed,
                                                     true, // interactableOnlyWhenCubeSeen
                                                     false, // interactableOnlyWhenKnownFaceSeen
                                                     "roll_cube_button",
                                                     DroneModeControlsSlide.ActionContextType.CubeNotInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.DropCubeButtonData,
                                                     HandleDropCubeButtonPressed,
                                                     false, // interactableOnlyWhenCubeSeen
                                                     false, // interactableOnlyWhenKnownFaceSeen
                                                     "drop_cube_button",
                                                     DroneModeControlsSlide.ActionContextType.CubeInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.StackCubeButtonData,
                                                     HandleStackCubeButtonPressed,
                                                     true, // interactableOnlyWhenCubeSeen
                                                     false, // interactableOnlyWhenKnownFaceSeen
                                                     "stack_cube_button",
                                                     DroneModeControlsSlide.ActionContextType.CubeInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.SayNameButtonData,
                                                     HandleSayNameButtonPressed,
                                                     false, // interactableOnlyWhenCubeSeen
                                                     true, // interactableOnlyWhenKnownFaceSeen
                                                     "say_name_button",
                                                     DroneModeControlsSlide.ActionContextType.FaceSeen);

          _DroneModeControlsSlide.OnDriveSpeedSegmentValueChanged += HandleDriveSpeedValueChanged;
          _DroneModeControlsSlide.OnDriveSpeedSegmentChanged += HandleDriveSpeedFamilyChanged;
          _DroneModeControlsSlide.OnHeadSliderValueChanged += HandleHeadSliderValueChanged;
          _DroneModeControlsSlide.OnLiftSliderValueChanged += HandleLiftSliderValueChanged;
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

          _RobotAnimator = new DroneModeTransitionAnimator(_CurrentRobot);
          _CurrentRobot.EnableDroneMode(true);
        }

        public override void Exit() {
          if (_CurrentRobot != null) {
            _CurrentRobot.StopAllMotors();
            _CurrentRobot.EnableDroneMode(false);
          }
          _DroneModeControlsSlide.OnDriveSpeedSegmentValueChanged -= HandleDriveSpeedValueChanged;
          _DroneModeControlsSlide.OnDriveSpeedSegmentChanged -= HandleDriveSpeedFamilyChanged;
          _DroneModeControlsSlide.OnHeadSliderValueChanged -= HandleHeadSliderValueChanged;
          _DroneModeControlsSlide.OnLiftSliderValueChanged -= HandleLiftSliderValueChanged;
          _DroneModeGame.OnTurnDirectionChanged -= HandleTurnDirectionChanged;
          DisableInput();
          _RobotAnimator.CleanUp();
        }

        public override void Update() {
          if (!_IsPerformingAction) {
            // Send drive wheels / drive head messages if needed
            SendDriveRobotMessages();

            _DroneModeControlsSlide.TiltText.text = TiltDrivingDebugText + HeadDrivingDebugText;
            _DroneModeControlsSlide.DebugText.text = _RobotAnimator.ToString();
          }
          else {
            SetSlidersToCurrentPosition();
          }
        }

        public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
          // Triggered by all reactionary behaviors. However, there is a special version of cliff events in engine. 
          // Also, these behaviors are only enabled when the player is not doing anything to Cozmo:
          //    Anki.Cozmo.BehaviorType.AcknowledgeFace
          //    Anki.Cozmo.BehaviorType.AcknowledgeObject
          //    Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement

          // Disable input so we can set slider values safely based on behavior activity
          DisableInput();
        }

        public override void PausedUpdate() {
          // Set sliders to new state since reactionary behaviors can change head and lift height
          SetSlidersToCurrentPosition();
        }

        public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
          // Set sliders to new state since reactionary behaviors can change head and lift height
          SetSlidersToCurrentPosition();

          // Re-enable input so that players can drive Cozmo again
          EnableInput();
        }

        private void SetSlidersToCurrentPosition() {
          float headAngleRadians = _CurrentRobot.HeadAngle;
          _DroneModeControlsSlide.SetHeadSliderValue(headAngleRadians * Mathf.Rad2Deg);
          _DroneModeControlsSlide.SetLiftSliderValue(_CurrentRobot.LiftHeightFactor);
        }

        private void EnableInput() {
          _DroneModeControlsSlide.EnableInput();
          _DroneModeGame.EnableTiltInput();
        }

        private void DisableInput() {
          _DroneModeControlsSlide.DisableInput();
          _DroneModeGame.DisableTiltInput();
        }

        #region Drive Robot
        private void SendDriveRobotMessages() {
          if (Time.time - _LastMessageSentTimestamp > _kSendMessageInterval_sec
              || ShouldStopDriving(_TargetDriveSpeed_mmps, _CurrentDriveSpeed_mmps, _TargetTurnDirection)) {
            _LastMessageSentTimestamp = Time.time;
            bool droveWheels = DriveWheelsIfNeeded();
            bool droveHead = DriveHeadIfNeeded();
            bool droveLift = DriveLiftIfNeeded();

            if (_IsDrivingHead != droveHead || _IsDrivingWheels != droveWheels || _IsDrivingLift != droveLift) {
              // If targets are both zero, enable reactionary behavior
              if (!droveHead && !droveWheels && !droveLift) {
                EnableIdleReactionaryBehaviors(true);
              }
              else {
                EnableIdleReactionaryBehaviors(false);
              }

              _IsDrivingHead = droveHead;
              _IsDrivingWheels = droveWheels;
              _IsDrivingLift = droveLift;
            }
          }
        }

        private void EnableIdleReactionaryBehaviors(bool enable) {
          _CurrentRobot.RequestEnableReactionaryBehavior("drone_mode", Anki.Cozmo.BehaviorType.AcknowledgeFace, enable);
          _CurrentRobot.RequestEnableReactionaryBehavior("drone_mode", Anki.Cozmo.BehaviorType.AcknowledgeObject, enable);
          _CurrentRobot.RequestEnableReactionaryBehavior("drone_mode", Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement, enable);
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
            _CurrentRobot.DriveWheels(0f, 0f);
            TiltDrivingDebugText = "Drive Arc: \nspeed mmps = " + 0 + " \nradius mm = N/A";
          }
          return droveWheels;
        }

        private bool DriveHeadIfNeeded() {
          bool startDriveHead = false;
          float currentHeadAngle_rad = _CurrentRobot.HeadAngle;
          if (!_IsDrivingHead && !currentHeadAngle_rad.IsNear(_TargetHeadAngle_rad, _kHeadFactorThreshold_rad)) {
            DriveHeadInternal();
            HeadDrivingDebugText = "\nPlayer driving head: target: " + _TargetHeadAngle_rad + " current=" + currentHeadAngle_rad;
            startDriveHead = true;
          }
          return startDriveHead;
        }

        private void HandleHeadMoveFinished(bool success) {
          float currentHeadAngle_rad = _CurrentRobot.HeadAngle;
          if (!currentHeadAngle_rad.IsNear(_TargetHeadAngle_rad, _kHeadFactorThreshold_rad)) {
            DriveHeadInternal();
            HeadDrivingDebugText = "\nPlayer driving head: target: " + _TargetHeadAngle_rad + " current=" + currentHeadAngle_rad;
          }
          else {
            _IsDrivingHead = false;
            HeadDrivingDebugText = "\nPlayer not driving head: target=" + _TargetHeadAngle_rad + " current=" + _CurrentRobot.GetHeadAngleFactor();
            _RobotAnimator.AllowIdleAnimation(true);
          }
        }

        private void DriveHeadInternal() {
          _CurrentRobot.CancelCallback(HandleHeadMoveFinished);
          _CurrentRobot.SetHeadAngle(_TargetHeadAngle_rad, callback: HandleHeadMoveFinished, useExactAngle: true,
                                     speed_radPerSec: _DroneModeGame.DroneModeConfigData.HeadTurnSpeed_radPerSec,
                                     accel_radPerSec2: _DroneModeGame.DroneModeConfigData.HeadTurnAccel_radPerSec2);

          _IsDrivingHead = true;
          _RobotAnimator.AllowIdleAnimation(false);
        }

        private bool DriveLiftIfNeeded() {
          bool startDriveLift = false;
          // Ideally we could do this check after every animation end, but this works for now.
          if (!_IsDrivingLift && !_CurrentRobot.LiftHeightFactor.IsNear(_TargetLiftFactor, _kLiftFactorThreshold)) {
            DriveLiftInternal();
            startDriveLift = true;
          }
          return startDriveLift;
        }

        private void HandleLiftMoveFinished(bool success) {
          if (!_CurrentRobot.LiftHeightFactor.IsNear(_TargetLiftFactor, _kLiftFactorThreshold)) {
            DriveLiftInternal();
          }
          else {
            _IsDrivingLift = false;
          }
        }

        private void DriveLiftInternal() {
          _CurrentRobot.CancelCallback(HandleLiftMoveFinished);
          _CurrentRobot.SetLiftHeight(_TargetLiftFactor,
                                      speed_radPerSec: _DroneModeGame.DroneModeConfigData.LiftTurnSpeed_radPerSec,
                                      accel_radPerSec2: _DroneModeGame.DroneModeConfigData.LiftTurnAccel_radPerSec2);

          _IsDrivingLift = true;
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

        private void HandleHeadSliderValueChanged(float newSliderValue_deg) {
          float newSliderValue_rad = newSliderValue_deg * Mathf.Deg2Rad;
          if (!newSliderValue_rad.IsNear(_TargetHeadAngle_rad, _kHeadFactorThreshold_rad)) {
            _TargetHeadAngle_rad = newSliderValue_rad;
          }
        }

        private void HandleLiftSliderValueChanged(float newSliderValue) {
          if (!newSliderValue.IsNear(_TargetLiftFactor, _kLiftFactorThreshold)) {
            _TargetLiftFactor = newSliderValue;
          }
        }

        private void HandleDriveSpeedFamilyChanged(DroneModeControlsSlide.SpeedSliderSegment currentPosition, DroneModeControlsSlide.SpeedSliderSegment newPosition) {
          _RobotAnimator.PlayTransitionAnimation(newPosition);
        }

        #endregion

        #region Contextual Actions

        private void HandleLiftCubeButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is ObservedObject && targetObject is LightCube) {
            _CurrentRobot.PickupObject(targetObject as ObservedObject, callback: HandleActionFinished);
            DisableInput();
            _IsPerformingAction = true;
          }
        }

        private void HandleRollCubeButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is ObservedObject && targetObject is LightCube) {
            _CurrentRobot.RollObject(targetObject as ObservedObject, callback: HandleActionFinished);
            DisableInput();
            _IsPerformingAction = true;
          }
        }

        private void HandleDropCubeButtonPressed() {
          _CurrentRobot.PlaceObjectOnGroundHere(callback: HandleActionFinished);
          DisableInput();
          _IsPerformingAction = true;
        }

        private void HandleStackCubeButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is ObservedObject && targetObject is LightCube) {
            _CurrentRobot.PlaceOnObject(targetObject as ObservedObject, callback: HandleActionFinished);
            DisableInput();
            _IsPerformingAction = true;
          }
        }

        private void HandleSayNameButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is Face) {
            _CurrentRobot.TurnTowardsLastFacePose(Mathf.PI, sayName: true, callback: HandleActionFinished);
            DisableInput();
            _IsPerformingAction = true;
          }
        }

        private void HandleActionFinished(bool success) {
          _CurrentRobot.CancelCallback(HandleActionFinished);
          EnableInput();
          _IsPerformingAction = false;
        }

        #endregion
      }
    }
  }
}