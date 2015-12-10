using UnityEngine;
using System.Collections;

namespace FaceTracking {
  // Perform Celebration, then shift to Tracking Face
  public class FoundFaceState : State {
    

    public override void Enter() {
      base.Enter();

      // Play Celebration upon entering state
      _CurrentRobot.SendAnimation(AnimationName.kSurprise,HandleAnimationEnd);
    }

    public void HandleAnimationEnd(bool success) {
      _StateMachine.SetNextState(new TrackFaceState());
    }

  }

}
