using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeState : State {

    #region Tunable values

    private int _AttemptsLeft = 3;
    private float _UnseenForgivenessSeconds = 2f;

    #endregion

    private LightCube _TargetCube;
    FollowCubeGame _GameInstance;

    private float _FirstUnseenTimestamp = -1;

    private bool _IsAnimatingFail = false;

    public override void Enter() {
      base.Enter();

      _GameInstance = _StateMachine.GetGame() as FollowCubeGame;
      _IsAnimatingFail = false;
      _AttemptsLeft = 3;
      _GameInstance.SetAttemptsLeft(_AttemptsLeft);
    }

    public override void Update() {
      base.Update();

      if (IsAnimating()) {
        return;
      }

      // Try to find a target
      if (_TargetCube == null) {
        _TargetCube = FindClosestLightCube();
      }

      // If there is a target, and it's currently in view, follow it.
      if (_TargetCube != null) {
        if (IsCubeVisible(_TargetCube)) {
          _TargetCube.SetLEDs(Color.white);
          FollowCube(_TargetCube);

          // TODO: Keep track of any change in direction.

          // TODO: If we have turned fully around in either direction, the player wins.
        }
        else {
          // Keep track of when Cozmo first loses track of the block
          if (IsUnseenTimestampUninitialized()) {
            _FirstUnseenTimestamp = Time.time;
          }

          if (Time.time - _FirstUnseenTimestamp > _UnseenForgivenessSeconds) {
            ResetUnseenTimestamp();
            // Only lose a life if some time passes
            // Lost sight of the target; the player loses a life!
            _TargetCube.TurnLEDsOff();
            _TargetCube = null;

            // TODO: Change this to an event?
            _AttemptsLeft--;
            _GameInstance.SetAttemptsLeft(_AttemptsLeft);

            if (_AttemptsLeft <= 0) {
              // TODO: Player lost; Move into fail state, then restart this state
            }
            else {
              _IsAnimatingFail = true;
              _CurrentRobot.SendAnimation("shocked", HandleLostCubeAnimationEnd);
            }
          }
          else {
            // Continue trying to follow the cube
            FollowCube(_TargetCube);
            _TargetCube.SetLEDs(Color.yellow);
          }
        }
      }
      else {
        // Don't move until we find a cube.
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
    }

    private bool IsAnimating() {
      return _IsAnimatingFail;
    }

    void HandleLostCubeAnimationEnd(bool success) {
      _IsAnimatingFail = false;

      // Set the head level
      _CurrentRobot.SetHeadAngle(0);
      _CurrentRobot.SetLiftHeight(0);
    }

    private LightCube FindClosestLightCube() {
      ObservedObject closest = null;
      float dist = float.MaxValue;
      foreach (ObservedObject obj in _CurrentRobot.VisibleObjects) {
        if (obj is LightCube) {
          float d = Vector3.Distance(_CurrentRobot.WorldPosition, obj.WorldPosition);
          if (d < dist) {
            dist = d;
            closest = obj;
          }
        }
      }
      LightCube cube = closest as LightCube;
      return cube;
    }

    private bool IsCubeVisible(LightCube cube) {
      return _CurrentRobot.VisibleObjects.Contains(cube);
    }

    void FollowCube(LightCube target) {
      float dist = Vector3.Distance(_CurrentRobot.WorldPosition, target.WorldPosition);
      float angle = Vector2.Angle(_CurrentRobot.Forward, target.WorldPosition - _CurrentRobot.WorldPosition);
      float speed = _GameInstance.ForwardSpeed;
      if (angle < 10.0f) {
        float distMax = _GameInstance.DistanceMax;
        float distMin = _GameInstance.DistanceMin;

        if (dist > distMax) {
          _CurrentRobot.DriveWheels(speed, speed);
        }
        else if (dist < distMin) {
          _CurrentRobot.DriveWheels(-speed, -speed);
        }
        else {
          _CurrentRobot.DriveWheels(0.0f, 0.0f);
        }
      }
      else {
        // we need to turn to face it
        bool shouldTurnRight = ShouldTurnRight(target);
        if (shouldTurnRight) {
          _CurrentRobot.DriveWheels(speed * 0.5f, -speed * 0.25f);
        }
        else {
          _CurrentRobot.DriveWheels(-speed * 0.25f, speed * 0.5f);
        }
      }

    }

    private bool ShouldTurnRight(ObservedObject followBlock) {
      float turnAngle = Vector3.Cross(_CurrentRobot.Forward, followBlock.WorldPosition - _CurrentRobot.WorldPosition).z;
      return (turnAngle < 0.0f);
    }

    private bool IsUnseenTimestampUninitialized() {
      return _FirstUnseenTimestamp == -1;
    }

    private void ResetUnseenTimestamp() {
      _FirstUnseenTimestamp = -1;
    }
  }

}
