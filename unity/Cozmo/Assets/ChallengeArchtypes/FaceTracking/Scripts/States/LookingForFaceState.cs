using UnityEngine;
using System.Collections;

namespace FaceTracking {
  // Default : Wander about, trying to find a face, goes to FoundFaceState if face is detected
  public class LookingForFaceState : State {
    
    // Make sure the face has been seen consistently to confirm that it is in fact a face
    private float _UnSeenResetDelay = 0.75f;
    private float _SeenHoldDelay = 1.0f;
    private float _FirstSeenTimestamp = -1;
    private float _FirstUnSeenTimestamp = -1;
    private FaceTrackingGame _GameInstance;
    private Face _TargetFace;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FaceTrackingGame;
      _GameInstance.MidCelebration = false;
      _TargetFace = null;
      _CurrentRobot.SetHeadAngle(0.35f);
      _CurrentRobot.SetLiftHeight(0.0f);
      // Entering this state resets progress as you've lost Cozmo's attention
      _GameInstance.StepsCompleted = 0.0f;
      _GameInstance.TargetLeft = false;
      _GameInstance.TargetRight = false;

      // Play Confused Animation as you enter this state, then begin
      // to wander aimlessly looking for a new face.
      if (_GameInstance.WanderEnabled) {
        _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      }
      else {
        _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      }

      _GameInstance.ShowNextSlide();
    }

    public override void Update() {
      base.Update();
      if (_GameInstance.MidCelebration) {
        return;
      }
      // Wander aimlessly
      // If a face has been found, enter FoundFaceState
      if (_CurrentRobot.Faces.Count > 0) {
        if (_TargetFace == null) {
          // If TargetFace is null, find the first valid face.
          for (int i = 0; i < _CurrentRobot.Faces.Count; i++) {
            if (_GameInstance.IsValidFace(_CurrentRobot.Faces[i])) {
              _TargetFace = _CurrentRobot.Faces[i];
              break;
            }
          }
        }
        // Only count a face as found if it is within the appropriate bounds
        if (_GameInstance.IsValidFace(_TargetFace)) {
          // We found a face, and its in the right spot!
          ResetUnSeenTimestamp();

          if (IsSeenTimestampUninitialized()) {
            _FirstSeenTimestamp = Time.time;
          }

          // Lock onto the face if they remain within ideal range for the full duration
          if (Time.time - _FirstSeenTimestamp > _SeenHoldDelay) {
            FindFace();
          }
        }
        else {
          // If the current face is not valid and there are multiple, grab the closest one
          if (_CurrentRobot.Faces.Count > 1) {
            _TargetFace = _GameInstance.ClosestFace();
          }
        }
      }
      else {
        if (IsUnSeenTimestampUninitialized()) {
          _FirstUnSeenTimestamp = Time.time;
        }
        if (Time.time - _FirstUnSeenTimestamp > _UnSeenResetDelay) {
          ResetSeenTimestamp();
          _TargetFace = null;
        }
      }
    }

    public void FindFace() {
      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, ProceduralEyeParameters.MakeDefaultLeftEye(), ProceduralEyeParameters.MakeDefaultRightEye());
      if (_TargetFace != null) {
        if (_GameInstance.WanderEnabled) {
          _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
        }
        _CurrentRobot.FacePose(_TargetFace);
      }

      if (_GameInstance.StepsCompleted == 0.0f) {
        _GameInstance.StepsCompleted += 0.333f;
      }
      _StateMachine.SetNextState(new AnimationState(AnimationName.kHappyA, HandleStateCompleteAnimationDone));
    }

    public void HandleStateCompleteAnimationDone(bool success) {
      _StateMachine.SetNextState(new TrackFaceState());
    }

    private bool IsSeenTimestampUninitialized() {
      return _FirstSeenTimestamp == -1;
    }

    private void ResetSeenTimestamp() {
      _FirstSeenTimestamp = -1;
    }

    private bool IsUnSeenTimestampUninitialized() {
      return _FirstUnSeenTimestamp == -1;
    }

    private void ResetUnSeenTimestamp() {
      _FirstUnSeenTimestamp = -1;
    }

  }

}
