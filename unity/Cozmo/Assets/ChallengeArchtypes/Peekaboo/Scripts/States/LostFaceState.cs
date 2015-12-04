using UnityEngine;
using System.Collections;

namespace Peekaboo {
  // Express Confusion, then enter LookingForFace state
  public class LostFaceState : State {
    

    public override void Enter() {
      base.Enter();

      // Express Confusion upon losing track of a face. probably something more perplexed than surprised
      _CurrentRobot.SendAnimation(AnimationName.kFinishTapCubeWin,HandleAnimationEnd);
    }

    public void HandleAnimationEnd(bool success) {
      _StateMachine.SetNextState(new LookingForFaceState());
    }

  }

}
