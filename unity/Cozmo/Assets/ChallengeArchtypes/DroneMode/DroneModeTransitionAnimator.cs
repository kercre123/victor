using Anki.Cozmo;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeTransitionAnimator {

    private IRobot _RobotToAnimate;

    private DroneModeView.SpeedSliderSegment _CurrentDriveSpeedSegment = DroneModeView.SpeedSliderSegment.Neutral;
    private DroneModeView.SpeedSliderSegment _TargetDriveSpeedSegment = DroneModeView.SpeedSliderSegment.Neutral;
    private TransitionAnimationState _CurrentAnimationState = TransitionAnimationState.NONE;

    private enum TransitionAnimationState {
      IN,
      OUT,
      NONE
    }

    public DroneModeTransitionAnimator(IRobot targetRobot) {
      _RobotToAnimate = targetRobot;

      // Push neutral idle
      _RobotToAnimate.PushIdleAnimation(AnimationTrigger.DroneModeIdle);
    }

    public void CleanUp() {
      // Pop neutral idle
      _RobotToAnimate.PopIdleAnimation();
      _RobotToAnimate.PopIdleAnimation();
      _RobotToAnimate.CancelCallback(HandleInAnimationFinished);
      _RobotToAnimate.CancelCallback(HandleOutAnimationFinished);
    }

    public void PlayTransitionAnimation(DroneModeView.SpeedSliderSegment newSliderSegment) {
      DAS.Info("DroneModeTransitionAnimator.PlayTransitionAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      _TargetDriveSpeedSegment = newSliderSegment;
      if (_CurrentAnimationState == TransitionAnimationState.NONE
          && _CurrentDriveSpeedSegment != _TargetDriveSpeedSegment) {
        PlayOutAnimation();
      }
    }

    private void PlayOutAnimation() {
      DAS.Info("DroneModeTransitionAnimator.PlayOutAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      _CurrentAnimationState = TransitionAnimationState.OUT;
      switch (_CurrentDriveSpeedSegment) {
      case DroneModeView.SpeedSliderSegment.Forward:
        // Play forward out
        if (_TargetDriveSpeedSegment == DroneModeView.SpeedSliderSegment.Turbo) {
          PlayInAnimation();
        }
        else {
          _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        }
        break;
      case DroneModeView.SpeedSliderSegment.Neutral:
        PlayInAnimation();
        break;
      case DroneModeView.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeView.SpeedSliderSegment.Turbo:
        if (_TargetDriveSpeedSegment == DroneModeView.SpeedSliderSegment.Neutral || _TargetDriveSpeedSegment == DroneModeView.SpeedSliderSegment.Reverse) {
          _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        }
        // else play turbo out? No plans for it yet
        break;
      default:
        // We should never get here.
        DAS.Error("DroneModeTransitionAnimator.PlayOutAnimation", "SpeedSliderSegment not implemented! " + _CurrentDriveSpeedSegment);
        break;
      }
    }

    private void HandleOutAnimationFinished(bool success) {
      DAS.Info("DroneModeTransitionAnimator.HandleOutAnimationFinished",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      // Pop current driving animation if not neutral
      if (_CurrentDriveSpeedSegment != DroneModeView.SpeedSliderSegment.Neutral) {
        _RobotToAnimate.PopIdleAnimation();
      }

      PlayInAnimation();

      // Check for success = false?
    }

    private void PlayInAnimation() {
      _CurrentDriveSpeedSegment = _TargetDriveSpeedSegment;
      DAS.Info("DroneModeTransitionAnimator.PlayInAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      _CurrentAnimationState = TransitionAnimationState.IN;
      switch (_TargetDriveSpeedSegment) {
      case DroneModeView.SpeedSliderSegment.Forward:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingStart, HandleInAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeView.SpeedSliderSegment.Neutral:
        HandleInAnimationFinished(true);
        break;
      case DroneModeView.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingStart, HandleInAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeView.SpeedSliderSegment.Turbo:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeTurboDrivingStart, HandleInAnimationFinished, QueueActionPosition.AT_END);
        break;
      default:
        // We should never get here.
        DAS.Error("DroneModeTransitionAnimator.PlayInAnimation", "SpeedSliderSegment not implemented! " + _TargetDriveSpeedSegment);
        break;
      }
    }

    private void HandleInAnimationFinished(bool success) {
      DAS.Info("DroneModeTransitionAnimator.HandleInAnimationFinished",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      SetDrivingAnimation();

      if (_CurrentDriveSpeedSegment == _TargetDriveSpeedSegment) {
        _CurrentAnimationState = TransitionAnimationState.NONE;
      }
      else {
        PlayOutAnimation();
      }

      // Check for success = false?
    }

    private void SetDrivingAnimation() {
      DAS.Info("DroneModeTransitionAnimator.SetDrivingAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment);
      // Push current driving animation if not neutral
      switch (_CurrentDriveSpeedSegment) {
      case DroneModeView.SpeedSliderSegment.Forward:
        _RobotToAnimate.PushIdleAnimation(AnimationTrigger.DroneModeForwardDrivingLoop);
        break;
      case DroneModeView.SpeedSliderSegment.Neutral:
        // Do nothing; it will default to idle
        break;
      case DroneModeView.SpeedSliderSegment.Reverse:
        _RobotToAnimate.PushIdleAnimation(AnimationTrigger.DroneModeBackwardDrivingLoop);
        break;
      case DroneModeView.SpeedSliderSegment.Turbo:
        _RobotToAnimate.PushIdleAnimation(AnimationTrigger.DroneModeForwardDrivingLoop);
        break;
      default:
        // We should never get here.
        DAS.Error("DroneModeTransitionAnimator.SetDrivingAnimation", "SpeedSliderSegment not implemented! " + _TargetDriveSpeedSegment);
        break;
      }
    }
  }
}