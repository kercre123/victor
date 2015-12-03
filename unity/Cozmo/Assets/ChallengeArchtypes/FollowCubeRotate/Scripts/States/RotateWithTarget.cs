using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class RotateWithTarget : State {

    private LightCube _Target;
    private float _LastTimeTargetSeen;
    private float _StartingAngle;
    private bool _LeftReached = false;
    private bool _RightReached = false;

    public void SetTarget(LightCube target) {
      _Target = target;
    }

    public override void Enter() {
      base.Enter();
      _StartingAngle = _CurrentRobot.PoseAngle;
    }

    public override void Update() {
      base.Update();

      if (_StartingAngle - _CurrentRobot.PoseAngle < Mathf.PI / 3.0f) {
        _LeftReached = true;
      }

      if (_StartingAngle - _CurrentRobot.PoseAngle > Mathf.PI / 3.0f) {
        _RightReached = true;
      }

      if (_LeftReached && _RightReached) {
        PlayWinAnimation();
        return;
      }

      if (_CurrentRobot.VisibleObjects.Contains(_Target)) {
        _LastTimeTargetSeen = Time.time;
      
        // the target is visible Follow it based on its pose.
        Vector3 targetToRobot = (_CurrentRobot.WorldPosition - _Target.WorldPosition).normalized;
        float crossValue = Vector3.Cross(targetToRobot, _CurrentRobot.Forward).z;

        // determines if the cube is to the left or right of cozmo.
        if (crossValue < 0.0f) {
          _CurrentRobot.DriveWheels(15.0f, -15.0f);
        }
        else {
          _CurrentRobot.DriveWheels(-15.0f, 15.0f);
        }

      }

      if (Time.time - _LastTimeTargetSeen > 2.0f) {
        // it's been too long since we've seen our target so let's start over...
        PlayLoseAnimation();
      }

    }

    private void PlayLoseAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kShocked, HandleLoseAnimationDone);
      _StateMachine.SetNextState(animationState);
    }

    private void PlayWinAnimation() {
      AnimationState animationState = new AnimationState();
      animationState.Initialize(AnimationName.kMajorWin, HandleWinAnimationDone);
      _StateMachine.SetNextState(animationState);
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
    }
  }
}