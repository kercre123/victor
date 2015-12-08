using UnityEngine;
using System.Collections;

namespace FollowCube {
  public class FollowCubeForwardState : State {

    private LightCube _CurrentTarget = null;
    private float _LastSeenTargetTime;
    private FollowCubeGame _GameInstance;
    private Vector3 _RobotStartPosition;
    private float _WinDistanceThreshold = 150.0f;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FollowCubeGame;
      _RobotStartPosition = _CurrentRobot.WorldPosition;
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
        _GameInstance.FailedAttempt();
        _StateMachine.SetNextState(new FollowCubeFailedState());
        return;
      }

      float distance = Vector3.Dot(_CurrentRobot.WorldPosition - _RobotStartPosition, _CurrentRobot.Forward);
      _GameInstance.Progress = (distance / _WinDistanceThreshold) * (1.0f / _GameInstance.NumSegments);
      if (distance > _WinDistanceThreshold) {
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
      float dist = Vector3.Distance(_CurrentRobot.WorldPosition, _CurrentTarget.WorldPosition);
      float speed = _GameInstance.ForwardSpeed;

      float distMax = _GameInstance.DistanceMax;
      float distMin = _GameInstance.DistanceMin;

      if (dist > distMax) {
        _CurrentRobot.DriveWheels(speed, speed);
      }
      else if (dist < distMin) {
        _CurrentRobot.DriveWheels(-speed, -speed);
      }
    }

    private void HandleTaskCompleteAnimation(bool success) {
      _StateMachine.SetNextState(new FollowCubeBackwardState());
      _GameInstance.CurrentFollowTask = FollowCubeGame.FollowTask.Backwards;
    }

    public override void Exit() {
      base.Exit();
    }
  }
}