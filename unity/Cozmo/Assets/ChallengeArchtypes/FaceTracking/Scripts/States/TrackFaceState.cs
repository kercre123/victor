using UnityEngine;
using System.Collections;

namespace FaceTracking {
  // Attempt to Follow the face along various axis
  // (Default should track in all directions at once)
  // Possibly define a specific tracking goal for Challenge/tutorial purposes
  public class TrackFaceState : State {

    #region Tunable values

    private float _UnseenForgivenessSeconds = 1.5f;

    #endregion
    
    private FaceTrackingGame _GameInstance;
    private float _FirstUnseenTimestamp = -1;

    private Face _TargetFace;

    private float _LastHeight;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FaceTrackingGame;
      // Trigger Scripted Sequence for Aria to tell player to tilt head left and right.
      // Score points when you do that, otherwise return to looking for face state if
      // you lose track. 
      // Track total distance tilted based on current goal that alternates between left and right.
      // Display a popup prompt for each
      _CurrentRobot.SetHeadAngle(0);
      _CurrentRobot.SetLiftHeight(0);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.InteractWithFaces);
    }

    public override void Update() {
      base.Update();
      // We've lost the face
      if (_CurrentRobot.Faces.Count <= 0) {
        // Keep track of when Cozmo first loses track of the face
        if (IsUnseenTimestampUninitialized()) {
          _FirstUnseenTimestamp = Time.time;
        }

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
        _TargetFace = _CurrentRobot.Faces[0];
        ResetUnseenTimestamp();
        FollowFace(_TargetFace);
      }
    }

    void FollowFace(Face target) {
      float dist = Vector3.Distance(_CurrentRobot.WorldPosition, target.WorldPosition);
      float angle = Vector2.Angle(_CurrentRobot.Forward, target.WorldPosition - _CurrentRobot.WorldPosition);
      float speed = _GameInstance.ForwardSpeed;

      // Determine the relative head angle and follow the face with Cozmo's head
      float height = Mathf.InverseLerp(30, 150, target.WorldPosition.z);

      if (Mathf.Abs(height) > 0.1f && Mathf.Abs(height) < 15.0f) {
        float delta = height - _LastHeight;
        _CurrentRobot.SetHeadAngle(_LastHeight+delta);
      }
      _LastHeight = height;

      if (angle < _GameInstance.TiltGoal) {
        _GameInstance.TiltSuccessCheck(target);
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

  }

}
