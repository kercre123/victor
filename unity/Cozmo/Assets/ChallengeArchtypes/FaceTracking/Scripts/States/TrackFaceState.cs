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

    #region eyestuff

    private Vector2 _LeftEyeOuterPosition;
    private Vector2 _RightEyeOuterPosition;

    private Vector2 _LeftEyeInnerPosition;
    private Vector2 _RightEyeInnerPosition;


    private ProceduralEyeParameters _LeftEye = ProceduralEyeParameters.MakeDefaultLeftEye();
    private ProceduralEyeParameters _RightEye = ProceduralEyeParameters.MakeDefaultRightEye();

    #endregion

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FaceTrackingGame;
      // Trigger Scripted Sequence for Aria to tell player to tilt head left and right.
      // Score points when you do that, otherwise return to looking for face state if
      // you lose track. 
      // Track total distance tilted based on current goal that alternates between left and right.
      // Display a popup prompt for each
      _CurrentRobot.SetHeadAngle(0.05f);
      _CurrentRobot.SetLiftHeight(0);

      _LeftEyeInnerPosition = _LeftEye.EyeCenter;
      _LeftEyeOuterPosition = _LeftEye.EyeCenter - new Vector2(_LeftEye.EyeCenter.x+20f,_LeftEye.EyeCenter.y);

      _RightEyeInnerPosition = _RightEye.EyeCenter;
      _RightEyeOuterPosition = _RightEye.EyeCenter + new Vector2(_RightEye.EyeCenter.x+20f,_RightEye.EyeCenter.y);
      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, _LeftEye, _RightEye);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      _TargetFace = null;
    }

    public override void Update() {
      base.Update();
      if (_GameInstance.MidCelebration) {
        return;
      }
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
      }// We have a face to track
      else {
        _TargetFace = _CurrentRobot.Faces[0];
        ResetUnseenTimestamp();
      }
      // Attempt to follow your face if you have one
      if (_TargetFace != null) {
        FollowFace(_TargetFace);
      }
    }

    void FollowFace(Face target) {
      float dist = Vector3.Distance(_CurrentRobot.WorldPosition, target.WorldPosition);
      float speed = _GameInstance.ForwardSpeed;

      // Drive closer if you pick up a face that's too far away
      if (dist > _GameInstance.DistanceMax) {
        _CurrentRobot.DriveWheels(speed, speed);
      } // Back up if you're too cloe.
      else if (dist < _GameInstance.DistanceMin) {
        _CurrentRobot.DriveWheels(speed, speed);
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }

      float targetAngleOffset = _GameInstance.CalculateTilt(target);
      if (Mathf.Abs(targetAngleOffset) <= 1.0f) {
        LerpEyes(Mathf.Clamp01(targetAngleOffset));
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

    public void LerpEyes(float lerpVal) {

      if (_GameInstance.TargetLeft) {
        _RightEye.EyeCenter = Vector2.Lerp(_RightEyeOuterPosition, _RightEyeInnerPosition, lerpVal);
        _LeftEye.EyeCenter = Vector2.Lerp(_LeftEyeInnerPosition, _LeftEyeOuterPosition, lerpVal);
      }
      else {
        _LeftEye.EyeCenter = Vector2.Lerp(_LeftEyeOuterPosition, _LeftEyeInnerPosition, lerpVal);
        _RightEye.EyeCenter = Vector2.Lerp(_RightEyeInnerPosition, _RightEyeOuterPosition, lerpVal);
      }

      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, _LeftEye, _RightEye);

    }

    private bool ShouldTurnRight(Face followFace) {
      float turnAngle = Vector3.Cross(_CurrentRobot.Forward, followFace.WorldPosition - _CurrentRobot.WorldPosition).z;
      return (turnAngle < 0.0f);
    }

    public void LoseFace() {
      _CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.white);
      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, ProceduralEyeParameters.MakeDefaultLeftEye(), ProceduralEyeParameters.MakeDefaultRightEye());
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
