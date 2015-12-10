using UnityEngine;
using System.Collections;

namespace FaceTracking {
  // Default : Wander about, trying to find a face, goes to FoundFaceState if face is detected
  public class LookingForFaceState : State {
    
    // Make sure the face has been seen consistently to confirm that it is in fact a face
    private float _SeenHoldDelay = 0.5f;
    private float _FirstSeenTimestamp = -1;
    private FaceTrackingGame _GameInstance;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as FaceTrackingGame;

      _CurrentRobot.SetHeadAngle(0);
      _CurrentRobot.SetLiftHeight(0);
      // Play Confused Animation as you enter this state, then begin
      // to wander aimlessly looking for a new face.

      if (_GameInstance.WanderEnabled) {
        _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      }
      else {
        _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      }

    }

    public override void Update() {
      base.Update();
      // Wander aimlessly
      // If a face has been found, enter FoundFaceState
      if (_CurrentRobot.Faces.Count > 0) {
        // We found a face!
        if (IsSeenTimestampUninitialized()) {
          _FirstSeenTimestamp = Time.time;
        }

        if (Time.time - _FirstSeenTimestamp > _SeenHoldDelay) {
          FindFace();
        }
      }
      else {
        ResetSeenTimestamp();
      }
    }

    public void FindFace() {
      _StateMachine.SetNextState(new FoundFaceState());
    }

    private bool IsSeenTimestampUninitialized() {
      return _FirstSeenTimestamp == -1;
    }

    private void ResetSeenTimestamp() {
      _FirstSeenTimestamp = -1;
    }
  }

}
