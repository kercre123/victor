using Anki.Cozmo;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeTransitionAnimator {

    private IRobot _RobotToAnimate;

    private DroneModeView.SpeedSliderSegment _CurrentDriveSpeedSegment = DroneModeView.SpeedSliderSegment.Neutral;
    private DroneModeView.SpeedSliderSegment _TargetDriveSpeedSegment = DroneModeView.SpeedSliderSegment.Neutral;
    private TransitionAnimationState _CurrentAnimationState = TransitionAnimationState.NONE;

    public delegate void TransitionAnimationsFinishedHandler();
    public event TransitionAnimationsFinishedHandler OnTransitionAnimationsFinished;

    private enum TransitionAnimationState {
      IN,
      OUT,
      NONE
    }

    public DroneModeTransitionAnimator(IRobot targetRobot) {
      _RobotToAnimate = targetRobot;

      // TODO Push neutral idle
    }

    public void CleanUp() {
      // TODO Pop neutral idle
    }

    public void PlayTransitionAnimation(DroneModeView.SpeedSliderSegment newSliderSegment) {
      _TargetDriveSpeedSegment = newSliderSegment;
      if (_CurrentAnimationState == TransitionAnimationState.NONE
          && _CurrentDriveSpeedSegment != _TargetDriveSpeedSegment) {
        PlayOutAnimation();
      }
    }

    private void PlayOutAnimation() {
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
      // TODO Pop current driving animation if not neutral

      PlayInAnimation();

      // Check for success = false?
    }

    private void PlayInAnimation() {
      _CurrentAnimationState = TransitionAnimationState.IN;
      _CurrentDriveSpeedSegment = _TargetDriveSpeedSegment;
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
      // TODO Push current driving animation if not neutral

      if (_CurrentDriveSpeedSegment == _TargetDriveSpeedSegment) {
        _CurrentAnimationState = TransitionAnimationState.NONE;
        if (OnTransitionAnimationsFinished != null) {
          OnTransitionAnimationsFinished();
        }
      }
      else {
        PlayOutAnimation();
      }

      // Check for success = false?
    }
  }


}