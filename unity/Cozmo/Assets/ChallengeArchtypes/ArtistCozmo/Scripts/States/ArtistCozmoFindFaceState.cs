using UnityEngine;
using System.Collections;
using Cozmo.Util;

namespace ArtistCozmo {
  public class ArtistCozmoFindFaceState : State {

    private const float _kMinTimeSeeingFace = 1f;

    private bool _HasSeenFace = false;

    private float _StartLookingAtFaceTime;

    public override void Enter() {
      base.Enter();
      ((ArtistCozmoGame)_StateMachine.GetGame()).RandomizeFilter();

      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.FindFaces);
    }

    public override void Update() {
      if (_CurrentRobot.Faces.Count > 0) {

        if (!_HasSeenFace) {
          _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
          _CurrentRobot.TurnTowardsFace(_CurrentRobot.Faces[0]);
          _HasSeenFace = true;
          _StartLookingAtFaceTime = Time.time;
        }
        else if (_StartLookingAtFaceTime + _kMinTimeSeeingFace < Time.time) {
          // Go to the next state
          _StateMachine.PushSubState(new ArtistCozmoSketchState());
        }
      }
      else {
        if (_HasSeenFace) {
          _HasSeenFace = false;
          _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.FindFaces);
        }
      }
    }

    public override void Exit() {
      base.Exit();
    }

  }

}