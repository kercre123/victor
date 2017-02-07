using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapPlayerGoalCozmoSelectCube : SpeedTapPlayerGoalBaseSelectCube {

    private const float _kPreDockPoseDistanceFromCube_mm = 120.0f;
    private const float _kCubeDistanceThreshold_mm = 45.0f;

    private bool _ForceRaiseLift = false;
    public const int ERROR_LOSTCUBE = 2;

    public override void Enter() {
      base.Enter();
      DAS.Debug("SpeedTapPlayerGoalCozmoSelectCube", "Enter");
      TryDrivingToCube(forceRaiseLift: false);
    }

    private void TryDrivingToCube(bool forceRaiseLift) {
      DAS.Debug("TryDrivingToCube", "_BlockID " + _BlockID);
      _ForceRaiseLift = forceRaiseLift;
      _CurrentRobot.SearchForCube(_BlockID, HandleSearchForCubeEnd);
      _CurrentRobot.PushDrivingAnimations(Anki.Cozmo.AnimationTrigger.SpeedTapDrivingStart,
        Anki.Cozmo.AnimationTrigger.SpeedTapDrivingLoop, Anki.Cozmo.AnimationTrigger.Count);
    }

    private void HandleSearchForCubeEnd(bool success) {
      DAS.Debug("HandleSearchForCubeEnd", "found " + success);
      if (success) {
        HandleFoundCube(_ForceRaiseLift);
      }
      else {
        // unable to find cube - bail out
        CompletePlayerGoal(ERROR_LOSTCUBE);
      }
    }

    private void HandleFoundCube(bool forceRaiseLift) {
      DAS.Debug("HandleFoundCube", "found " + forceRaiseLift);
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

    private LightCube GetBlock() {
      LightCube cube = null;
      if (_CurrentRobot != null) {
        _CurrentRobot.LightCubes.TryGetValue(_BlockID, out cube);
      }
      return cube;
    }

    private bool IsFarAwayFromCube() {
      return (_CurrentRobot.WorldPosition.xy() - GetBlock().WorldPosition.xy()).sqrMagnitude
      >= (_kPreDockPoseDistanceFromCube_mm * _kPreDockPoseDistanceFromCube_mm);
    }

    private bool IsReallyCloseToCube() {
      return (_CurrentRobot.WorldPosition.xy() - GetBlock().WorldPosition.xy()).sqrMagnitude
      <= (_kCubeDistanceThreshold_mm * _kCubeDistanceThreshold_mm);
    }

    private void DriveToPreDockPose() {
      DAS.Debug("DriveToPreDockPose", "found ");
      _CurrentRobot.GotoObject(GetBlock(), _kPreDockPoseDistanceFromCube_mm,
        goToPreDockPose: true, callback: HandleDriveToPreDockPoseComplete);
    }

    private void HandleDriveToPreDockPoseComplete(bool success) {
      DAS.Debug("HandleDriveToPreDockPoseComplete", "found " + success);
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
      DAS.Debug("HandleLiftRaiseComplete", "found " + success);
      if (success) {
        DriveToCube();
      }
      else {
        RaiseLift();
      }
    }

    private void DriveToCube() {
      _CurrentRobot.AlignWithObject(GetBlock(), 0.0f, HandleDriveToCubeComplete,
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
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSpeedtapCozmoConfirm, HandleTapDone);
      if (OnCubeSelected != null) {
        OnCubeSelected(_ParentPlayer, _BlockID);
      }

    }

    private void HandleTapDone(bool success) {
      CompletePlayerGoal(SUCCESS);
    }

  }
}