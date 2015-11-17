using UnityEngine;
using System.Collections;

namespace Vortex {

  // This might just be a state showing hte rules or something..
  public class StateIntro : State {

    private bool _AnimationPlaying = false;

    public override void Enter() {
      base.Enter();
      DAS.Info(this, "StateIntro");
      _AnimationPlaying = true;
      _CurrentRobot.SendAnimation("majorWin", AnimationDone);
    }

    public override void Update() {
      base.Update();

      if (!_AnimationPlaying) {
        _StateMachine.SetNextState(new StateRequestSpin());
      }
    }

    public override void Exit() {
      base.Exit();
    }

    void AnimationDone(bool success) {
      _AnimationPlaying = false;
    }
      
  }

}
