using UnityEngine;
using System.Collections;

namespace Peekaboo {
  // Perform Celebration, then shift to Tracking Face
  public class FoundFaceState : State {
    
    PeekGame _GameInstance;

    public override void Enter() {
      base.Enter();

      _GameInstance = _StateMachine.GetGame() as PeekGame;
      // Play Celebration upon entering state
      _CurrentRobot.SendAnimation(AnimationName.kShocked,HandleAnimationEnd);
    }

    public void HandleAnimationEnd(bool success) {
      _StateMachine.SetNextState(new TrackFaceState());
    }

  }

}
