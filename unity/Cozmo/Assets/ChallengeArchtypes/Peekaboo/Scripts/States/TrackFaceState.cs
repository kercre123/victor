using UnityEngine;
using System.Collections;

namespace Peekaboo {
  // Attempt to Follow the face along various axis
  // (Default should track in all directions at once)
  // Possibly define a specific tracking goal for Challenge/tutorial purposes
  public class TrackFaceState : State {

    #region Tunable values

    private float _UnseenForgivenessSeconds = 0.75f;
    private float _SeenScoreDelay = 2.0f;

    #endregion
    
    PeekGame _GameInstance;
    private float _FirstUnseenTimestamp = -1;
    private float _FirstSeenTimestamp = -1;
    private bool _hasScored;

    private Face _TargetFace;

    private float _PreviousAnglePose;
    private float _TotalRadiansTraveled;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as PeekGame;
      _hasScored = false;

      _CurrentRobot.SetHeadAngle(0);
      _CurrentRobot.SetLiftHeight(0);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    }

    public override void Update() {
      base.Update();
      // We've lost the face
      if (_CurrentRobot.Faces.Count <= 0) {
        // Keep track of when Cozmo first loses track of the face
        if (IsUnseenTimestampUninitialized()) {
          _FirstUnseenTimestamp = Time.time;
        }
        ResetSeenTimestamp();

        if (Time.time - _FirstUnseenTimestamp > _UnseenForgivenessSeconds) {
          ResetUnseenTimestamp();
          LoseFace();
        }
        else {
          // Continue trying to follow the last seen face
          if (_TargetFace != null) {
            FollowFace(_TargetFace);
          }
        }
      }// We have a face to track
      else {
        // Keep track of when how long we've seen the face for scoring
        if (IsSeenTimestampUninitialized() && _hasScored == false) {
          _FirstSeenTimestamp = Time.time;
        }
        _TargetFace = _CurrentRobot.Faces[0];
        ResetUnseenTimestamp();

        if (Time.time - _FirstSeenTimestamp > _SeenScoreDelay && _hasScored == false) {
          _hasScored = true;
          _GameInstance.PeekSuccess();
          _CurrentRobot.SendAnimation(AnimationName.kMajorWin);
        }

        _CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.green);

        FollowFace(_TargetFace);
        // Keep track of any change in direction.
        float deltaRadians = _PreviousAnglePose - _CurrentRobot.PoseAngle;
        // Disregard a huge change in rotation, because that means he's passing the 
        // border from -pi to pi.
        if (Mathf.Abs(deltaRadians) < 1) {
          _TotalRadiansTraveled += deltaRadians;
        }
        _PreviousAnglePose = _CurrentRobot.PoseAngle;
      }
    }

    void FollowFace(Face target) {
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
          _CurrentRobot.DriveWheels(speed, -speed);
        }
        else {
          _CurrentRobot.DriveWheels(-speed, speed);
        }
      }
    }

    private bool ShouldTurnRight(Face followFace) {
      float turnAngle = Vector3.Cross(_CurrentRobot.Forward, followFace.WorldPosition - _CurrentRobot.WorldPosition).z;
      return (turnAngle < 0.0f);
    }

    public void LoseFace() {
      _CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.white);
      _StateMachine.SetNextState(new LostFaceState());
    }

    private bool IsUnseenTimestampUninitialized() {
      return _FirstUnseenTimestamp == -1;
    }

    private void ResetUnseenTimestamp() {
      _FirstUnseenTimestamp = -1;
    }

    private bool IsSeenTimestampUninitialized() {
      return _FirstSeenTimestamp == -1;
    }

    private void ResetSeenTimestamp() {
      _FirstSeenTimestamp = -1;
    }

  }

}
