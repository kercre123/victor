using UnityEngine;
using System.Collections;

namespace Peekaboo {
  // Express Confusion, then enter LookingForFace state
  public class LostFaceState : State {
    
    PeekGame _GameInstance;

    public override void Enter() {
      base.Enter();

      _GameInstance = _StateMachine.GetGame() as PeekGame;

      // Express Confusion upon losing track of a face. probably something more perplexed than surprised
      _CurrentRobot.SendAnimation(AnimationName.kFinishTapCubeWin,HandleAnimationEnd);
    }

    public void HandleAnimationEnd(bool success) {
      _StateMachine.SetNextState(new LookingForFaceState());
    }

  }

}
