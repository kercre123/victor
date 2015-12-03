using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class RotateWithTarget : State {

    private FollowCubeRotateGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as FollowCubeRotateGame;
    }

    public override void Update() {
      base.Update();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);

      if (!_Game.LeftReached && _Game.StartingAngle - _CurrentRobot.PoseAngle < -(Mathf.PI / 4.0f)) {
        _Game.LeftReached = true;
        PlayPartialWinAnimation();
      }

      if (!_Game.RightReached && _Game.StartingAngle - _CurrentRobot.PoseAngle > Mathf.PI / 4.0f) {
        _Game.RightReached = true;
        PlayPartialWinAnimation();
      }

      if (_Game.LeftReached && _Game.RightReached) {
        PlayWinAnimation();
        return;
      }

      if (_CurrentRobot.VisibleObjects.Contains(_Game.CurrentTarget)) {

        // the target is visible Follow it based on its pose.
        Vector3 targetToRobot = (_CurrentRobot.WorldPosition - _Game.CurrentTarget.WorldPosition).normalized;
        float crossValue = Vector3.Cross(targetToRobot, _CurrentRobot.Forward).z;

        // determines if the cube is to the left or right of cozmo.
        if (crossValue < 0.0f && !_Game.RightReached) {
          _CurrentRobot.DriveWheels(15.0f, -15.0f);
        }

        if (crossValue > 0.0f && !_Game.LeftReached) {
          _CurrentRobot.DriveWheels(-15.0f, 15.0f);
        }

      }

      if (Time.time - _Game.LastTimeTargetSeen > 2.0f) {
        // it's been too long since we've seen our target so let's start over...
        PlayLoseAnimation();
      }

    }

    private void PlayLoseAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kShocked, HandleLoseAnimationDone);
      _StateMachine.SetNextState(animationState);
      _Game.CurrentTarget = null;
    }

    private void PlayPartialWinAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kEnjoyLight, HandlePartialWinAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void PlayWinAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kMajorWin, HandleWinAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void HandlePartialWinAnimationDone(bool success) {
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _StateMachine.SetNextState(new RotateWithTarget());
    }

    private void HandleLoseAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForNewCube());
      (_StateMachine.GetGame() as FollowCubeRotateGame).LoseLife();
    }

    private void HandleWinAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }
}