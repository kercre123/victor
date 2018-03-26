using Anki.Cozmo;

namespace Cozmo.Challenge.DroneMode {
  public class DroneModeTransitionAnimator {
    public event System.Action OnTurboTransitionAnimationStarted;
    public event System.Action OnTurboTransitionAnimationFinished;

    private IRobot _RobotToAnimate;

    private DroneModeControlsSlide.SpeedSliderSegment _CurrentDriveSpeedSegment = DroneModeControlsSlide.SpeedSliderSegment.Neutral;
    private DroneModeControlsSlide.SpeedSliderSegment _TargetDriveSpeedSegment = DroneModeControlsSlide.SpeedSliderSegment.Neutral;
    private TransitionAnimationState _CurrentAnimationState = TransitionAnimationState.NONE;

    private string _DebugString;

    private bool _AllowIdleAnimation = true;

    private const string kDroneModeIdle = "drone_mode_transition_idle";

    public enum TransitionAnimationState {
      IN,
      OUT,
      NONE
    }

    public DroneModeTransitionAnimator(IRobot targetRobot) {
      _RobotToAnimate = targetRobot;

      // Push neutral idle
      PushRobotIdleAnimation(AnimationTrigger.DroneModeIdle);
    }

    public void CleanUp() {
      _RobotToAnimate.CancelCallback(HandleInAnimationFinished);
      _RobotToAnimate.CancelCallback(HandleOutAnimationFinished);
    }

    public void PlayTransitionAnimation(DroneModeControlsSlide.SpeedSliderSegment newSliderSegment) {
      UpdateDebugStringAndSendDAS("PlayTransitionAnimation");
      _TargetDriveSpeedSegment = newSliderSegment;
      if (_CurrentDriveSpeedSegment != _TargetDriveSpeedSegment) {
        if (_TargetDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Neutral) {
          PlayOutAnimation(_CurrentDriveSpeedSegment, _TargetDriveSpeedSegment);
        }
        else {
          PlayInAnimation(_TargetDriveSpeedSegment);
        }
      }
    }

    private void PlayOutAnimation(DroneModeControlsSlide.SpeedSliderSegment speedToLeave, DroneModeControlsSlide.SpeedSliderSegment speedToEnter) {
      UpdateDebugStringAndSendDAS("PlayOutAnimation");
      _CurrentAnimationState = TransitionAnimationState.OUT;
      switch (speedToLeave) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        // Play forward out
        if (speedToEnter == DroneModeControlsSlide.SpeedSliderSegment.Turbo) {
          HandleOutAnimationFinished(true);
        }
        else {
          _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.NOW_AND_CLEAR_REMAINING);
        }
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        PlayInAnimation(speedToEnter);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.NOW_AND_CLEAR_REMAINING);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
        if (speedToEnter == DroneModeControlsSlide.SpeedSliderSegment.Neutral || speedToEnter == DroneModeControlsSlide.SpeedSliderSegment.Reverse) {
          _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.NOW_AND_CLEAR_REMAINING);
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
      UpdateDebugStringAndSendDAS("HandleOutAnimationFinished");
      PlayInAnimation(_TargetDriveSpeedSegment);
    }

    private void PlayInAnimation(DroneModeControlsSlide.SpeedSliderSegment speedToEnter) {
      _CurrentDriveSpeedSegment = speedToEnter;
      UpdateDebugStringAndSendDAS("PlayInAnimation");
      _CurrentAnimationState = TransitionAnimationState.IN;
      SetDrivingAnimation(_CurrentDriveSpeedSegment);
      switch (speedToEnter) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingStart, HandleInAnimationFinished, QueueActionPosition.NOW_AND_CLEAR_REMAINING);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        HandleInAnimationFinished(true);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingStart, HandleInAnimationFinished, QueueActionPosition.NOW_AND_CLEAR_REMAINING);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeTurboDrivingStart, HandleInAnimationFinished, QueueActionPosition.NOW_AND_CLEAR_REMAINING);
        if (OnTurboTransitionAnimationStarted != null) {
          OnTurboTransitionAnimationStarted();
        }
        break;
      default:
        // We should never get here.
        DAS.Error("DroneModeTransitionAnimator.PlayInAnimation", "SpeedSliderSegment not implemented! " + _TargetDriveSpeedSegment);
        break;
      }
    }

    private void HandleInAnimationFinished(bool success) {
      if (success) {
        if (_CurrentDriveSpeedSegment == _TargetDriveSpeedSegment) {
          _CurrentAnimationState = TransitionAnimationState.NONE;
          if (_TargetDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Turbo && OnTurboTransitionAnimationFinished != null) {
            OnTurboTransitionAnimationFinished();
          }
        }
        else {
          PlayOutAnimation(_CurrentDriveSpeedSegment, _TargetDriveSpeedSegment);
        }

        UpdateDebugStringAndSendDAS("HandleInAnimationFinished");
      }
    }

    private void SetDrivingAnimation(DroneModeControlsSlide.SpeedSliderSegment currentDriveType) {
      UpdateDebugStringAndSendDAS("SetDrivingAnimation");
      // Push current driving animation if not neutral
      switch (currentDriveType) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        PushRobotIdleAnimation(AnimationTrigger.DroneModeForwardDrivingLoop);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        UpdateDroneModeIdleAnimation();
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        PushRobotIdleAnimation(AnimationTrigger.DroneModeBackwardDrivingLoop);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
        PushRobotIdleAnimation(AnimationTrigger.DroneModeForwardDrivingLoop);
        break;
      default:
        // We should never get here.
        DAS.Error("DroneModeTransitionAnimator.SetDrivingAnimation", "SpeedSliderSegment not implemented! " + _TargetDriveSpeedSegment);
        break;
      }
    }

    public void AllowIdleAnimation(bool allow) {
      if (allow != _AllowIdleAnimation) {
        _AllowIdleAnimation = allow;
        if (_CurrentDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Neutral
            && _TargetDriveSpeedSegment == DroneModeControlsSlide.SpeedSliderSegment.Neutral) {
          UpdateDroneModeIdleAnimation();
        }
      }
    }

    private void UpdateDroneModeIdleAnimation() {
      if (_AllowIdleAnimation) {
        PushRobotIdleAnimation(AnimationTrigger.DroneModeIdle);
      }
      else {
        // Has no head angle keyframes
        PushRobotIdleAnimation(AnimationTrigger.DroneModeKeepAlive);
      }
    }

    private void PushRobotIdleAnimation(AnimationTrigger animation) {
      _RobotToAnimate.PushIdleAnimation(animation, kDroneModeIdle);
    }

    private void PopRobotIdleAnimation() {
    }

    private void UpdateDebugStringAndSendDAS(string methodTag) {
      DAS.Info("DroneModeTransitionAnimator." + methodTag,
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      _DebugString = "DroneModeTransitionAnimator." + methodTag
        + "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
    }
  }
}