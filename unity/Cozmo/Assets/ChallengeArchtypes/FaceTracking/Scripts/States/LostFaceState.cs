using UnityEngine;
using System.Collections;

namespace FaceTracking {
  // Express Confusion, then enter LookingForFace state
  public class LostFaceState : State {
    

    public override void Enter() {
      base.Enter();
      _CurrentRobot.SetHeadAngle(0);

      // Express Confusion upon losing track of a face. probably something more perplexed than surprised
      _CurrentRobot.SendAnimation(AnimationName.kByeBye,HandleAnimationEnd);
    }

    public void HandleAnimationEnd(bool success) {
      _StateMachine.SetNextState(new LookingForFaceState());
    }

  }

}
