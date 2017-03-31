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
        private float _LastCommandedTargetHeadAngle_rad;

        private float _TargetLiftFactor;
        private float _LastCommandedTargetLiftFactor;

        private float _LastMessageSentTimestamp;

        private bool _IsDrivingWheels = false;
        private bool _IsDrivingHead = false;
        private bool _IsDrivingLift = false;

        private bool _IsPerformingAction = false;
        private bool _AreIdleReactionaryBehaviorsEnabled = true;

        private bool IsDrivingWheels {
          get { return _IsDrivingWheels; }
          set {
            if (_IsDrivingWheels != value) {
              _IsDrivingWheels = value;
              UpdateIdleReactionaryBehaviors();
              _DroneModeControlsSlide.ShowActionButtons = !_IsDrivingWheels;
            }
          }
        }

        private bool IsDrivingHead {
          get { return _IsDrivingHead; }
          set {
            if (_IsDrivingHead != value) {
              _IsDrivingHead = value;
              UpdateIdleReactionaryBehaviors();
            }
          }
        }

        private bool IsDrivingLift {
          get { return _IsDrivingLift; }
          set {
            if (_IsDrivingLift != value) {
              _IsDrivingLift = value;
              UpdateIdleReactionaryBehaviors();
            }
          }
        }

        private bool IsPerformingAction {
          get { return _IsPerformingAction; }
          set {
            if (_IsPerformingAction != value) {
              _IsPerformingAction = value;
              UpdateIdleReactionaryBehaviors();
              _DroneModeControlsSlide.AllowChangeFocus = !_IsPerformingAction;
            }
          }
        }

        private DroneModeTransitionAnimator _RobotAnimator;
        private bool _IsPlayingTurboTransitionAnimation = false;

        public string TiltDrivingDebugText = "";
        public string HeadDrivingDebugText = "";

        public override void Enter() {
          // We need to allow folks to touch both sliders at the same time
          Input.multiTouchEnabled = true;

          _LastMessageSentTimestamp = Time.time;

          _DroneModeGame = _StateMachine.GetGame() as DroneModeGame;
          _DroneModeGame.OnTurnDirectionChanged += HandleTurnDirectionChanged;

          _TargetLiftFactor = _DroneModeGame.DroneModeConfigData.StartingLiftHeight;
          _CurrentRobot.SetLiftHeight(_TargetLiftFactor);

          GameObject slide = _DroneModeGame.SharedMinigameView.ShowFullScreenGameStateSlide(
                               _DroneModeGame.DroneModeViewPrefab.gameObject, "drone_mode_view_slide");
          _DroneModeGame.SharedMinigameView.HideTitleWidget();
          _DroneModeControlsSlide = slide.GetComponent<DroneModeControlsSlide>();
          _DroneModeControlsSlide.InitializeCameraFeed(_CurrentRobot);
          _DroneModeControlsSlide.InitializeLiftSlider(_TargetLiftFactor);
          _DroneModeControlsSlide.InitializeHeadSlider(_CurrentRobot.HeadAngle * Mathf.Rad2Deg);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.LiftCubeButtonData,
            HandleLiftCubeButtonPressed,
            true, // interactableOnlyWhenCubeSeen
            false, // interactableOnlyWhenFaceSeen
            "lift_cube_button",
            DroneModeControlsSlide.ActionContextType.CubeNotInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.RollCubeButtonData,
            HandleRollCubeButtonPressed,
            true, // interactableOnlyWhenCubeSeen
            false, // interactableOnlyWhenFaceSeen
            "roll_cube_button",
            DroneModeControlsSlide.ActionContextType.CubeNotInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.DropCubeButtonData,
            HandleDropCubeButtonPressed,
            false, // interactableOnlyWhenCubeSeen
            false, // interactableOnlyWhenFaceSeen
            "drop_cube_button",
            DroneModeControlsSlide.ActionContextType.CubeInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.StackCubeButtonData,
            HandleStackCubeButtonPressed,
            true, // interactableOnlyWhenCubeSeen
            false, // interactableOnlyWhenFaceSeen
            "stack_cube_button",
            DroneModeControlsSlide.ActionContextType.CubeInLift);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.SayNameButtonData,
            HandleSayNameButtonPressed,
            false, // interactableOnlyWhenCubeSeen
            true, // interactableOnlyWhenFaceSeen
            "say_name_button",
            DroneModeControlsSlide.ActionContextType.FaceSeen);

          _DroneModeControlsSlide.CreateActionButton(_DroneModeGame.DroneModeConfigData.ReactToPetButtonData,
            HandleReactToPetButtonPressed,
            false, // interactableOnlyWhenCubeSeen
            false, // interactableOnlyWhenFaceSeen
            "react_to_pet_button",
            DroneModeControlsSlide.ActionContextType.PetSeen);

          UIManager.Instance.BackgroundColorController.SetBackgroundColor(UI.BackgroundColorController.BackgroundColor.TintMe,
            _DroneModeControlsSlide.BackgroundColor);
          _DroneModeGame.SharedMinigameView.HideMiddleBackground();

          // DroneModeControlsSlide implements its own quit button so hide the shared one 
          _DroneModeGame.SharedMinigameView.HideQuitButton();
          _DroneModeControlsSlide.OnQuitConfirmed += _DroneModeGame.SharedMinigameView.HandleQuitConfirmed;

          // Show how to play when the player plays drone mode for the first time
          int timesPlayedDroneMode = 0;
          DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.TryGetValue(_DroneModeGame.ChallengeID, out timesPlayedDroneMode);
          if (timesPlayedDroneMode <= 0) {
            _DroneModeControlsSlide.OpenHowToPlayModal(showCloseButton: false, playAnimations: true);
          }

          // Send get in animation; do not accept input while animation is playing
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.DroneModeGetIn, SetUpRobotAnimations);
          DisableInput();
        }

        private void SetUpRobotAnimations(bool getInSuccess) {
          _RobotAnimator = new DroneModeTransitionAnimator(_CurrentRobot);
          _RobotAnimator.OnTurboTransitionAnimationStarted += HandleTurboTransitionAnimationFinished;
          _RobotAnimator.OnTurboTransitionAnimationFinished += HandleTurboTransitionAnimationFinished;

          // HandleDriveSpeedFamilyChanged depends on RobotAnimator being set up
          _DroneModeControlsSlide.OnDriveSpeedSegmentChanged += HandleDriveSpeedFamilyChanged;

          // Accept input after Get In animation is done
          _DroneModeControlsSlide.OnDriveSpeedSegmentValueChanged += HandleDriveSpeedValueChanged;
          _DroneModeControlsSlide.OnHeadSliderValueChanged += HandleHeadSliderValueChanged;
          _DroneModeControlsSlide.OnLiftSliderValueChanged += HandleLiftSliderValueChanged;
          EnableInput();

          _CurrentRobot.EnableDroneMode(true);
          _CurrentRobot.SetEnableFreeplayLightStates(true);
          _CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kDroneModeDriveId, ReactionaryBehaviorEnableGroups.kDroneModeDriveTriggers);
        }

        public override void Exit() {
          // Revert to default after allowing folks to touch both sliders at the same time
          Input.multiTouchEnabled = false;

          if (_CurrentRobot != null) {
            _CurrentRobot.StopAllMotors();
            _CurrentRobot.EnableDroneMode(false);
            _CurrentRobot.SetEnableFreeplayLightStates(false);
            _CurrentRobot.SetNightVision(false);
            _CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kDroneModeDriveId);
          }
          _DroneModeControlsSlide.OnDriveSpeedSegmentValueChanged -= HandleDriveSpeedValueChanged;
          _DroneModeControlsSlide.OnDriveSpeedSegmentChanged -= HandleDriveSpeedFamilyChanged;
          _DroneModeControlsSlide.OnHeadSliderValueChanged -= HandleHeadSliderValueChanged;
          _DroneModeControlsSlide.OnLiftSliderValueChanged -= HandleLiftSliderValueChanged;
          _DroneModeGame.OnTurnDirectionChanged -= HandleTurnDirectionChanged;
          DisableInput();
          _RobotAnimator.OnTurboTransitionAnimationStarted -= HandleTurboTransitionAnimationFinished;
          _RobotAnimator.OnTurboTransitionAnimationFinished -= HandleTurboTransitionAnimationFinished;
          _RobotAnimator.CleanUp();
        }

        public override void Update() {
          if (!IsPerformingAction) {
            // Wait until the initial get in animation is done before accepting input
            if (_RobotAnimator != null) {
              // Send drive wheels / drive head messages if needed
              SendDriveRobotMessages();
              if (_IsPlayingTurboTransitionAnimation) {
                SetHeadSliderToCurrentPosition();
              }

              _DroneModeControlsSlide.TiltText.text = TiltDrivingDebugText + HeadDrivingDebugText;

              _DroneModeControlsSlide.DebugText.text = _RobotAnimator.ToString();
            }
            else {
              // Have sliders follow get in animation
              SetSlidersToCurrentPosition();
            }
          }
          else {
            SetSlidersToCurrentPosition();
          }
        }

        public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
          // Triggered by all reactionary behaviors. However, there is a special version of cliff events in engine. 
          // Also, these behaviors are only enabled when the player is not doing anything to Cozmo:
          //    Anki.Cozmo.BehaviorType.AcknowledgeFace
          //    Anki.Cozmo.BehaviorType.AcknowledgeObject
          //    Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement

          if (reactionaryBehavior == Anki.Cozmo.ReactionTrigger.PlacedOnCharger) {
            _CurrentRobot.DriveWheels(0f, 0f);
            _RobotAnimator.AllowIdleAnimation(false);
          }

          // Disable input so we can set slider values safely based on behavior activity
          DisableInput();
        }

        public override void PausedUpdate() {
          // Set sliders to new state since reactionary behaviors can change head and lift height
          SetSlidersToCurrentPosition();
        }

        public override void Resume(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
          // Set sliders to new state since reactionary behaviors can change head and lift height
          SetSlidersToCurrentPosition();

          if (reactionaryBehavior == Anki.Cozmo.ReactionTrigger.PlacedOnCharger) {
            _RobotAnimator.AllowIdleAnimation(true);
          }

          // Re-enable input so that players can drive Cozmo again
          EnableInput();
        }

        private void SetSlidersToCurrentPosition() {
          SetHeadSliderToCurrentPosition();
          _DroneModeControlsSlide.SetLiftSliderValue(_CurrentRobot.LiftHeightFactor);
        }

        private void SetHeadSliderToCurrentPosition() {
          if (_CurrentRobot != null && _DroneModeControlsSlide != null) {
            float headAngleRadians = _CurrentRobot.HeadAngle;
            _DroneModeControlsSlide.SetHeadSliderValue(headAngleRadians * Mathf.Rad2Deg);
          }
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
            IsDrivingWheels = DriveWheelsIfNeeded();

            if (!_IsPlayingTurboTransitionAnimation) {
              DriveHeadIfNeeded();
            }

            DriveLiftIfNeeded();
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
            _CurrentRobot.DriveWheels(0f, 0f);
            TiltDrivingDebugText = "Drive Arc: \nspeed mmps = " + 0 + " \nradius mm = N/A";
          }
          return droveWheels;
        }

        private void DriveHeadIfNeeded() {
          float currentHeadAngle_rad = _CurrentRobot.HeadAngle;
          if (!_LastCommandedTargetHeadAngle_rad.IsNear(_TargetHeadAngle_rad, _kHeadFactorThreshold_rad)) {
            DriveHeadInternal();
            HeadDrivingDebugText = "\nPlayer driving head: target: " + _TargetHeadAngle_rad + " current=" + currentHeadAngle_rad;
          }
        }

        private void HandleHeadMoveFinished(bool success) {
          IsDrivingHead = false;
          HeadDrivingDebugText = "\nPlayer not driving head: target=" + _TargetHeadAngle_rad + " current=" + _CurrentRobot.GetHeadAngleFactor();
          _RobotAnimator.AllowIdleAnimation(true);
        }

        private void DriveHeadInternal() {
          _CurrentRobot.CancelCallback(HandleHeadMoveFinished);
          _LastCommandedTargetHeadAngle_rad = _TargetHeadAngle_rad;
          _CurrentRobot.SetHeadAngle(_TargetHeadAngle_rad, callback: HandleHeadMoveFinished,
            queueActionPosition: Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING,
            useExactAngle: true,
            speed_radPerSec: _DroneModeGame.DroneModeConfigData.HeadTurnSpeed_radPerSec,
            accel_radPerSec2: _DroneModeGame.DroneModeConfigData.HeadTurnAccel_radPerSec2);

          IsDrivingHead = true;
          _RobotAnimator.AllowIdleAnimation(false);
        }

        private void DriveLiftIfNeeded() {
          // Ideally we could do this check after every animation end, but this works for now.
          if (!_LastCommandedTargetLiftFactor.IsNear(_TargetLiftFactor, _kLiftFactorThreshold)) {
            DriveLiftInternal();
          }
        }

        private void HandleLiftMoveFinished(bool success) {
          IsDrivingLift = false;
        }

        private void DriveLiftInternal() {
          _CurrentRobot.CancelCallback(HandleLiftMoveFinished);
          _LastCommandedTargetLiftFactor = _TargetLiftFactor;
          _CurrentRobot.SetLiftHeight(_TargetLiftFactor,
            callback: HandleLiftMoveFinished,
            queueActionPosition: Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING,
            speed_radPerSec: _DroneModeGame.DroneModeConfigData.LiftTurnSpeed_radPerSec,
            accel_radPerSec2: _DroneModeGame.DroneModeConfigData.LiftTurnAccel_radPerSec2);

          IsDrivingLift = true;
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

        private void HandleTurboTransitionAnimationStarted() {
          _IsPlayingTurboTransitionAnimation = true;
          _DroneModeControlsSlide.DisableHeadSlider();
        }

        private void HandleTurboTransitionAnimationFinished() {
          _IsPlayingTurboTransitionAnimation = false;
          _DroneModeControlsSlide.EnableHeadSlider();
        }

        #endregion

        #region Contextual Actions

        private void HandleLiftCubeButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is LightCube) {
            _CurrentRobot.DriveWheels(0f, 0f); // In case drive commands are being sent, thereby locking the wheels
            _CurrentRobot.PickupObject(targetObject as ObservableObject, checkForObjectOnTop: false, callback: HandlePickupActionFinished);
            DisableInput();
            IsPerformingAction = true;
          }
        }

        private void HandleRollCubeButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is LightCube) {
            _CurrentRobot.DriveWheels(0f, 0f); // In case drive commands are being sent, thereby locking the wheels
            _CurrentRobot.RollObject(targetObject as ObservableObject, checkForObjectOnTop: false, callback: HandleActionFinished);
            DisableInput();
            IsPerformingAction = true;
          }
        }

        private void HandleDropCubeButtonPressed() {
          _CurrentRobot.DriveWheels(0f, 0f); // In case drive commands are being sent, thereby locking the wheels

          // Need to give the stop from DriveWheels a chance to actually stop the robot so that PlaceObjectOnGround
          // doesn't fail due to IMU still reporting the robot is turning. So we first wait for 0.1sec and then place on ground.
          Anki.Cozmo.ExternalInterface.RobotActionUnion[] actions = {
            new Anki.Cozmo.ExternalInterface.RobotActionUnion().Initialize(new Anki.Cozmo.ExternalInterface.Wait().Initialize(0.1f)),
            new Anki.Cozmo.ExternalInterface.RobotActionUnion().Initialize(new Anki.Cozmo.ExternalInterface.PlaceObjectOnGroundHere())
          };

          _CurrentRobot.SendQueueCompoundAction(actions, callback: HandleActionFinished);

          DisableInput();
          IsPerformingAction = true;
        }

        private void HandleStackCubeButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is LightCube) {
            _CurrentRobot.DriveWheels(0f, 0f); // In case drive commands are being sent, thereby locking the wheels
            _CurrentRobot.PlaceOnObject(targetObject as ObservableObject, checkForObjectOnTop: false, callback: HandleActionFinished);
            DisableInput();
            IsPerformingAction = true;
          }
        }

        private void HandleSayNameButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is Face) {
            float maxTurnAngle_rad = Mathf.PI;
            if (_CurrentRobot.Status(Anki.Cozmo.RobotStatusFlag.IS_PICKED_UP)) {
              maxTurnAngle_rad = 0f; // Don't try to turn at all if picked up
            }

            _CurrentRobot.DriveWheels(0f, 0f); // In case drive commands are being sent, thereby locking the wheels
            _CurrentRobot.TurnTowardsFace((Face)targetObject,
              maxTurnAngle_rad: maxTurnAngle_rad,
              sayName: true,
              namedTrigger: Anki.Cozmo.AnimationTrigger.AcknowledgeFaceNamed,
              unnamedTrigger: Anki.Cozmo.AnimationTrigger.AcknowledgeFaceUnnamed,
              callback: HandleActionFinished);

            DisableInput();
            IsPerformingAction = true;
          }

        }

        private void HandleReactToPetButtonPressed() {
          IVisibleInCamera targetObject = _DroneModeControlsSlide.CurrentlyFocusedObject;
          if (targetObject != null && targetObject is PetFace) {
            _CurrentRobot.DriveWheels(0f, 0f); // In case drive commands are being sent, thereby locking the wheels
            Anki.Cozmo.AnimationTrigger reactionAnimation = (((PetFace)targetObject).PetType == Anki.Vision.PetType.Cat) ?
              Anki.Cozmo.AnimationTrigger.PetDetectionCat : Anki.Cozmo.AnimationTrigger.PetDetectionDog;
            _CurrentRobot.SendAnimationTrigger(reactionAnimation,
              callback: HandleActionFinished,
              ignoreBodyTrack: _CurrentRobot.Status(Anki.Cozmo.RobotStatusFlag.IS_PICKED_UP));
            DisableInput();
            IsPerformingAction = true;
          }
        }

        private void HandleActionFinished(bool success) {
          _CurrentRobot.CancelCallback(HandleActionFinished);
          if (!_StateMachine.IsPaused) {
            EnableInput();
          }
          IsPerformingAction = false;
        }

        private void HandlePickupActionFinished(bool success) {
          _DroneModeControlsSlide.HandlePickupActionResult(success);
          HandleActionFinished(success);
        }

        #endregion

        private void UpdateIdleReactionaryBehaviors() {
          // If targets are all zero, enable reactionary behavior
          if (!IsDrivingHead && !IsDrivingWheels && !IsDrivingLift && !IsPerformingAction) {
            if (!_AreIdleReactionaryBehaviorsEnabled) {
              EnableIdleReactionaryBehaviors(true);
              _AreIdleReactionaryBehaviorsEnabled = true;
            }
          }
          else {
            if (_AreIdleReactionaryBehaviorsEnabled) {
              EnableIdleReactionaryBehaviors(false);
              _AreIdleReactionaryBehaviorsEnabled = false;
            }
          }
        }

        private void EnableIdleReactionaryBehaviors(bool enable) {
          if (_CurrentRobot != null) {
            if (!enable) {
              _CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kDroneModeIdleId, ReactionaryBehaviorEnableGroups.kDroneModeIdleTriggers);
            }
            else {
              _CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kDroneModeIdleId);
            }
          }
        }
      }
    }
  }
}
