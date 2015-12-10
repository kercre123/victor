using UnityEngine;
using System.Collections;

namespace FaceTracking {
  // Attempt to Follow the face along various axis
  // (Default should track in all directions at once)
  // Possibly define a specific tracking goal for Challenge/tutorial purposes
  public class TrackFaceState : State {

    #region Tunable values

    private float _UnseenForgivenessSeconds = 1.0f;

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
      _GameInstance.MidCelebration = false;
      // Trigger Scripted Sequence for Aria to tell player to tilt head left and right.
      // Score points when you do that, otherwise return to looking for face state if
      // you lose track. 
      // Track total distance tilted based on current goal that alternates between left and right.
      // Display a popup prompt for each
      _CurrentRobot.SetLiftHeight(0);

      _LeftEyeInnerPosition = _LeftEye.EyeCenter;
      _LeftEyeOuterPosition = _LeftEye.EyeCenter + new Vector2(_LeftEye.EyeCenter.x-20f,_LeftEye.EyeCenter.y);

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

      if (_CurrentRobot.Faces.Count > 0) {
        // TODO: Potentially compare new face to old face and calculate distance to
        // determine if the new face is actually what we should follow or if its
        // an additional face introduced
        _TargetFace = _CurrentRobot.Faces[0];
        ResetUnseenTimestamp();
      }
      // Check to see if we've lost the face
      if (_CurrentRobot.Faces.Count <= 0 || !_GameInstance.IsValidFace(_TargetFace)) {
        // Keep track of when Cozmo first loses track of the face
        if (IsUnseenTimestampUninitialized()) {
          _FirstUnseenTimestamp = Time.time;
        }

        if (Time.time - _FirstUnseenTimestamp > _UnseenForgivenessSeconds) {
          ResetUnseenTimestamp();
          _TargetFace = null;
          LoseFace();
        }
      }
      // Attempt to follow your face if you have one
      if (_TargetFace != null) {
        FollowFace(_TargetFace);
      }
    }

    void FollowFace(Face target) {
      // Lerp eyes if the identified face is within an acceptable range.
      // Follow with body if the face is too far.
      float targetAngleOffset = _GameInstance.CalculateTilt(target);
      if (Mathf.Abs(targetAngleOffset) <= 1.0f) {
        LerpEyesHoriz(targetAngleOffset);
      }
    }

    public void LerpEyesHoriz(float lerpVal) {
      _LeftEye.UpperLidY = 0.5f;
      _RightEye.UpperLidY = 0.5f;
      _RightEye.EyeCenter = Vector2.Lerp(_RightEyeOuterPosition, _RightEyeInnerPosition, lerpVal);
      _LeftEye.EyeCenter = Vector2.Lerp(_LeftEyeInnerPosition, _LeftEyeOuterPosition, lerpVal);

      if (lerpVal > 0.0f) {
        _RightEye.UpperLidY = 0.0f;
      }
      else {
        _LeftEye.UpperLidY =  0.0f;
      }

      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, _LeftEye, _RightEye);
    }

    public void LoseFace() {
      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, ProceduralEyeParameters.MakeDefaultLeftEye(), ProceduralEyeParameters.MakeDefaultRightEye());
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kByeBye, HandleStateCompleteAnimationDone);
      _StateMachine.SetNextState(animState);
    }

    public void HandleStateCompleteAnimationDone(bool success) {
      _StateMachine.SetNextState(new LookingForFaceState());
    }

    private bool IsUnseenTimestampUninitialized() {
      return _FirstUnseenTimestamp == -1;
    }

    private void ResetUnseenTimestamp() {
      _FirstUnseenTimestamp = -1;
    }

  }

}
