using UnityEngine;
using System.Collections;

namespace FollowCube {
  public class FollowCubeTurnRightState : State {

    private LightCube _CurrentTarget = null;
    private float _LastSeenTargetTime;
    private FollowCubeGame _GameInstance;
    private float _RobotStartAngle;
    private float _WinDistanceThreshold = Mathf.PI / 4.0f;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FollowCubeGame;
      _RobotStartAngle = _CurrentRobot.PoseAngle;
      _LastSeenTargetTime = Time.time;
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
    }

    public override void Update() {
      base.Update();

      if (_CurrentTarget == null && _CurrentRobot.VisibleObjects.Count > 0) {
        _CurrentTarget = _CurrentRobot.VisibleObjects[0] as LightCube;
        _CurrentTarget.SetLEDs(CozmoPalette.ColorToUInt(Color.white));
      }

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      if (Time.time - _LastSeenTargetTime > _GameInstance.NotSeenForgivenessThreshold) {
        _StateMachine.SetNextState(new FollowCubeFailedState());
        return;
      }

      float angleDelta = _CurrentRobot.PoseAngle - _RobotStartAngle;
      _GameInstance.Progress = (-angleDelta / _WinDistanceThreshold) * (1.0f / _GameInstance.NumSegments) + (3.0f / _GameInstance.NumSegments);
      if (angleDelta < -(_WinDistanceThreshold)) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kEnjoyPattern, HandleTaskCompleteAnimation);
        _StateMachine.SetNextState(animState);
      }
      else if (_CurrentRobot.VisibleObjects.Contains(_CurrentTarget)) {
        _LastSeenTargetTime = Time.time;
        FollowTarget();
      }

    }

    private void FollowTarget() {
      // the target is visible Follow it based on its pose.
      Vector3 targetToRobot = (_CurrentRobot.WorldPosition - _CurrentTarget.WorldPosition).normalized;
      float crossValue = Vector3.Cross(targetToRobot, _CurrentRobot.Forward).z;

      if (crossValue < 0.0f) {
        _CurrentRobot.DriveWheels(15.0f, -15.0f);
      }
    }

    private void HandleTaskCompleteAnimation(bool success) {
      _StateMachine.SetNextState(new FollowCubeDriveState());
      _GameInstance.CurrentFollowTask = FollowCubeGame.FollowTask.FollowDrive;
    }

    public override void Exit() {
      base.Exit();
    }
  }
}