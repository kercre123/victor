using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapCozmoDriveToCube : State {

    private const float _kPreDockPoseDistanceFromCube_mm = 120.0f;
    private const float _kCubeDistanceThreshold_mm = 45.0f;

    private SpeedTapGame _SpeedTapGame = null;

    private bool _IsFirstTime = false;

    public SpeedTapCozmoDriveToCube(bool isFirstTime) {
      _IsFirstTime = isFirstTime;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      if (_IsFirstTime) {
        _SpeedTapGame.InitialCubesDone();
      }

      _SpeedTapGame.DeregisterUnwantedInterruptionEvents();

      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _SpeedTapGame.StartCycleCube(_SpeedTapGame.CozmoBlock, 
        Cozmo.CubePalette.TapMeColor.lightColors, 
        Cozmo.CubePalette.TapMeColor.cycleIntervalSeconds);

      _SpeedTapGame.ShowWaitForCozmoSlide();
      _SpeedTapGame.SharedMinigameView.ShowMiddleBackground();
      _SpeedTapGame.SharedMinigameView.ShowSpinnerWidget();

      _CurrentRobot.SetDrivingAnimations(AnimationGroupName.kSpeedTap_Driving_Start, 
        AnimationGroupName.kSpeedTap_Driving_Loop, null);
      if (IsFarAwayFromCube()) {
        DriveToPreDockPose();
      }
      else if (IsReallyCloseToCube()) {
        CompleteDriveToCube();
      }
      else {
        RaiseLift();
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.ResetDrivingAnimations();
      _SpeedTapGame.SharedMinigameView.HideSpinnerWidget();
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
      _CurrentRobot.SendAnimationGroup(AnimationGroupName.kSpeedTap_Driving_End, HandleLiftRaiseComplete);
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
      _SpeedTapGame.RegisterUnwantedInterruptionEvents();

      _StateMachine.SetNextState(new SpeedTapCozmoConfirm());
    }
  }

}
