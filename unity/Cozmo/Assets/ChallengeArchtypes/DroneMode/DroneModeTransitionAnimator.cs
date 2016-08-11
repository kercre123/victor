using Anki.Cozmo;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeTransitionAnimator {

    private IRobot _RobotToAnimate;

    private DroneModeControlsSlide.SpeedSliderSegment _CurrentDriveSpeedSegment = DroneModeControlsSlide.SpeedSliderSegment.Neutral;
    private DroneModeControlsSlide.SpeedSliderSegment _TargetDriveSpeedSegment = DroneModeControlsSlide.SpeedSliderSegment.Neutral;
    private TransitionAnimationState _CurrentAnimationState = TransitionAnimationState.NONE;

    private string _DebugString;
    private System.Collections.Generic.Stack<AnimationTrigger> _IdleAnimationStack;

    public enum TransitionAnimationState {
      IN,
      OUT,
      NONE
    }

    public DroneModeTransitionAnimator(IRobot targetRobot) {
      _IdleAnimationStack = new System.Collections.Generic.Stack<AnimationTrigger>();
      _RobotToAnimate = targetRobot;

      // Push neutral idle
      PushRobotIdleAnimation(AnimationTrigger.DroneModeIdle);
    }

    public void CleanUp() {
      // Pop neutral idle
      while (_IdleAnimationStack.Count > 0) {
        PopRobotIdleAnimation();
      }
      _RobotToAnimate.CancelCallback(HandleInAnimationFinished);
      _RobotToAnimate.CancelCallback(HandleOutAnimationFinished);
    }

    public override string ToString() {
      string animationStack = "";
      foreach (var anim in _IdleAnimationStack) {
        animationStack += "\n";
        animationStack += anim.ToString();
      }
      return _DebugString + "\nStack(" + _IdleAnimationStack.Count + "):" + animationStack;
    }

    public void PlayTransitionAnimation(DroneModeControlsSlide.SpeedSliderSegment newSliderSegment) {
      UpdateDebugStringAndSendDAS("PlayTransitionAnimation");
      _TargetDriveSpeedSegment = newSliderSegment;
      if (_CurrentAnimationState == TransitionAnimationState.NONE
          && _CurrentDriveSpeedSegment != _TargetDriveSpeedSegment) {
        PlayOutAnimation(_CurrentDriveSpeedSegment, _TargetDriveSpeedSegment);
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
          _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeForwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        }
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        PlayInAnimation(speedToEnter);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Reverse:
        _RobotToAnimate.SendAnimationTrigger(AnimationTrigger.DroneModeBackwardDrivingEnd, HandleOutAnimationFinished, QueueActionPosition.AT_END);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Turbo:
        if (speedToEnter == DroneModeControlsSlide.SpeedSliderSegment.Neutral || speedToEnter == DroneModeControlsSlide.SpeedSliderSegment.Reverse) {
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
      UpdateDebugStringAndSendDAS("HandleOutAnimationFinished");
      // Pop current driving animation if not neutral
      PopRobotIdleAnimation();

      PlayInAnimation(_TargetDriveSpeedSegment);

      // Check for success = false?
    }

    private void PlayInAnimation(DroneModeControlsSlide.SpeedSliderSegment speedToEnter) {
      _CurrentDriveSpeedSegment = speedToEnter;
      UpdateDebugStringAndSendDAS("PlayInAnimation");
      _CurrentAnimationState = TransitionAnimationState.IN;
      switch (speedToEnter) {
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
      SetDrivingAnimation();

      if (_CurrentDriveSpeedSegment == _TargetDriveSpeedSegment) {
        _CurrentAnimationState = TransitionAnimationState.NONE;
      }
      else {
        PlayOutAnimation(_CurrentDriveSpeedSegment, _TargetDriveSpeedSegment);
      }

      UpdateDebugStringAndSendDAS("HandleInAnimationFinished");
    }

    private void SetDrivingAnimation() {
      UpdateDebugStringAndSendDAS("SetDrivingAnimation");
      // Push current driving animation if not neutral
      switch (_CurrentDriveSpeedSegment) {
      case DroneModeControlsSlide.SpeedSliderSegment.Forward:
        PushRobotIdleAnimation(AnimationTrigger.DroneModeForwardDrivingLoop);
        break;
      case DroneModeControlsSlide.SpeedSliderSegment.Neutral:
        // Pop animations until we only have an idle
        while (_IdleAnimationStack.Count > 0) {
          PopRobotIdleAnimation();
        }

        if (_IdleAnimationStack.Count <= 0) {
          PushRobotIdleAnimation(AnimationTrigger.DroneModeIdle);
        }
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

    private void PushRobotIdleAnimation(AnimationTrigger animation) {
      _RobotToAnimate.PushIdleAnimation(animation);
      _IdleAnimationStack.Push(animation);
    }

    private void PopRobotIdleAnimation() {
      _RobotToAnimate.PopIdleAnimation();
      _IdleAnimationStack.Pop();
    }

    private void UpdateDebugStringAndSendDAS(string methodTag) {
      DAS.Info("DroneModeTransitionAnimator." + methodTag,
               "State: " + _CurrentAnimationState + "   Current:" + _CurrentDriveSpeedSegment + "   Target:" + _TargetDriveSpeedSegment);
      _DebugString = "DroneModeTransitionAnimator." + methodTag
        + "\nState: " + _CurrentAnimationState + "\nCurrent:" + _CurrentDriveSpeedSegment + "\nTarget:" + _TargetDriveSpeedSegment;
    }
  }
}