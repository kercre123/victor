using Cozmo.UI;

namespace SpeedTap {

  public class SpeedTapCozmoDriveToCube : State {

    private const float _kPreDockPoseDistanceFromCube_mm = 120.0f;
    private const float _kCubeDistanceThreshold_mm = 45.0f;

    private SpeedTapGame _SpeedTapGame = null;

    private bool _IsFirstTime = false;

    private bool _ForceRaiseLift = false;

    public SpeedTapCozmoDriveToCube(bool isFirstTime) {
      _IsFirstTime = isFirstTime;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      if (_IsFirstTime) {
        _SpeedTapGame.InitialCubesDone();
      }

      _SpeedTapGame.StartCycleCube(_SpeedTapGame.CozmoBlock,
                                   CubePalette.Instance.TapMeColor.lightColors,
                                   CubePalette.Instance.TapMeColor.cycleIntervalSeconds);

      _SpeedTapGame.SharedMinigameView.ShowShelf();
      _SpeedTapGame.ShowWaitForCozmoSlide();
      _SpeedTapGame.SharedMinigameView.HideMiddleBackground();
      _SpeedTapGame.SharedMinigameView.ShowSpinnerWidget();
      _SpeedTapGame.SharedMinigameView.HideHowToPlayButton();

      RobotEngineManager.Instance.CurrentRobot.SetEnableCliffSensor(true);

      TryDrivingToCube(forceRaiseLift: false);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.PopDrivingAnimations();
      _SpeedTapGame.SharedMinigameView.HideSpinnerWidget();
      _CurrentRobot.CancelAllCallbacks();
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // Cancel all callbacks
      _CurrentRobot.PopDrivingAnimations();
      _CurrentRobot.CancelAllCallbacks();
    }

    public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // Try driving up to the cube again
      TryDrivingToCube(forceRaiseLift: true);
    }

    private void TryDrivingToCube(bool forceRaiseLift) {
      _ForceRaiseLift = forceRaiseLift;
      _CurrentRobot.SearchForCube(_SpeedTapGame.CozmoBlock, HandleSearchForCubeEnd);
      _CurrentRobot.PushDrivingAnimations(Anki.Cozmo.AnimationTrigger.SpeedTapDrivingStart,
        Anki.Cozmo.AnimationTrigger.SpeedTapDrivingLoop, Anki.Cozmo.AnimationTrigger.Count);
    }

    private void HandleSearchForCubeEnd(bool success) {
      if (success) {
        HandleFoundCube(_ForceRaiseLift);
      }
      else {
        // unable to find cube - bail out
        _SpeedTapGame.ShowInterruptionQuitGameView(LocalizationKeys.kMinigameLostTrackOfBlockTitle,
          LocalizationKeys.kMinigameLostTrackOfBlockDescription);
      }
    }

    private void HandleFoundCube(bool forceRaiseLift) {
      if (IsFarAwayFromCube()) {
        DriveToPreDockPose();
      }
      else if (IsReallyCloseToCube() && !forceRaiseLift) {
        CompleteDriveToCube();
      }
      else {
        RaiseLift();
      }
    }

    private bool IsFarAwayFromCube() {
      return (_CurrentRobot.WorldPosition.xy() - _SpeedTapGame.CozmoBlock.WorldPosition.xy()).sqrMagnitude
      >= (_kPreDockPoseDistanceFromCube_mm * _kPreDockPoseDistanceFromCube_mm);
    }

    private bool IsReallyCloseToCube() {
      return (_CurrentRobot.WorldPosition.xy() - _SpeedTapGame.CozmoBlock.WorldPosition.xy()).sqrMagnitude
      <= (_kCubeDistanceThreshold_mm * _kCubeDistanceThreshold_mm);
    }

    private void DriveToPreDockPose() {
      _CurrentRobot.GotoObject(_SpeedTapGame.CozmoBlock, _kPreDockPoseDistanceFromCube_mm,
        goToPreDockPose: true, callback: HandleDriveToPreDockPoseComplete);
    }

    private void HandleDriveToPreDockPoseComplete(bool success) {
      if (success) {
        RaiseLift();
      }
      else {
        DriveToPreDockPose();
      }
    }

    private void RaiseLift() {
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.SpeedTapDrivingEnd, HandleLiftRaiseComplete);
    }

    private void HandleLiftRaiseComplete(bool success) {
      if (success) {
        DriveToCube();
      }
      else {
        RaiseLift();
      }
    }

    private void DriveToCube() {
      _CurrentRobot.AlignWithObject(_SpeedTapGame.CozmoBlock, 0.0f, HandleDriveToCubeComplete,
        useApproachAngle: false, usePreDockPose: false, alignmentType: Anki.Cozmo.AlignmentType.BODY);
    }

    private void HandleDriveToCubeComplete(bool success) {
      if (success) {
        CompleteDriveToCube();
      }
      else {
        DriveToCube();
      }
    }

    private void CompleteDriveToCube() {
      _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
    }
  }

}
