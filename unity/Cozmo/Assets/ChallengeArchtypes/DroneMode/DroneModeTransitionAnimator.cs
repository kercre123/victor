using Anki.Cozmo;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeTransitionAnimator {

    private IRobot _RobotToAnimate;

    private DroneModeControlsSlide.SpeedSliderSegment _CurrentDriveSpeedSegment = DroneModeControlsSlide.SpeedSliderSegment.Neutral;
    private DroneModeControlsSlide.SpeedSliderSegment _TargetDriveSpeedSegment = DroneModeControlsSlide.SpeedSliderSegment.Neutral;
    private TransitionAnimationState _CurrentAnimationState = TransitionAnimationState.NONE;

    private string _DebugString;

    public enum TransitionAnimationState {
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

    public override string ToString() {
      return _DebugString;
    }

    public void PlayTransitionAnimation(DroneModeControlsSlide.SpeedSliderSegment newSliderSegment) {
      DAS.Info("DroneModeTransitionAnimator.PlayTransitionAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      //_DebugString = "DroneModeTransitionAnimator.PlayTransitionAnimation" +
      //       "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
      _TargetDriveSpeedSegment = newSliderSegment;
      if (_CurrentAnimationState == TransitionAnimationState.NONE
          && _CurrentDriveSpeedSegment != _TargetDriveSpeedSegment) {
        PlayOutAnimation();
      }
    }

    private void PlayOutAnimation() {
      DAS.Info("DroneModeTransitionAnimator.PlayOutAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      //_DebugString = "DroneModeTransitionAnimator.PlayOutAnimation" +
      //        "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
      _CurrentAnimationState = TransitionAnimationState.OUT;
      switch (_CurrentDriveSpeedSegment) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        // Play forward out
        if (_TargetDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Turbo) {
          PlayInAnimation();
        }
        else {
          _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        }
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        PlayInAnimation();
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
        if (_TargetDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Neutral || _TargetDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Reverse) {
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
      _DebugString = "DroneModeTransitionAnimator.HandleOutAnimationFinished" +
               "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
      // Pop current driving animation if not neutral
      if (_CurrentDriveSpeedSegment != DroneModeControlsSlide.SpeedSliderSegment.Neutral) {
        _RobotToAnimate.PopIdleAnimation();
      }

      PlayInAnimation();

      // Check for success = false?
    }

    private void PlayInAnimation() {
      _CurrentDriveSpeedSegment = _TargetDriveSpeedSegment;
      DAS.Info("DroneModeTransitionAnimator.PlayInAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      //_DebugString = "DroneModeTransitionAnimator.PlayInAnimation" +
      //        "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
      _CurrentAnimationState = TransitionAnimationState.IN;
      switch (_TargetDriveSpeedSegment) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingStart, HandleInAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        HandleInAnimationFinished(true);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingStart, HandleInAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
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
      _DebugString = "DroneModeTransitionAnimator.HandleInAnimationFinished" +
               "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
    }

    private void SetDrivingAnimation() {
      DAS.Info("DroneModeTransitionAnimator.SetDrivingAnimation",
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment);
      //_DebugString = "DroneModeTransitionAnimator.SetDrivingAnimation" +
      // "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
      // Push current driving animation if not neutral
      switch (_CurrentDriveSpeedSegment) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        _RobotToAnimate.PushIdleAnimation(AnimationTrigger.DroneModeForwardDrivingLoop);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        // Do nothing; it will default to idle
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        _RobotToAnimate.PushIdleAnimation(AnimationTrigger.DroneModeBackwardDrivingLoop);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
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