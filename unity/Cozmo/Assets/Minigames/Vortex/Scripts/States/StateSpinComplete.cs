using UnityEngine;
using System.Collections;

namespace Vortex {
  /*
   * This state waits for the animation on cozmo to be done while players look at the score.
   */
  public class StateSpinComplete : State {
    private bool _DidCozmoWin;

    public StateSpinComplete(bool DidCosmoWin = false) : base() {
      _DidCozmoWin = DidCosmoWin;
    }

    public override void Enter() {
      base.Enter();
      if (_DidCozmoWin) {
        _CurrentRobot.SendAnimation("majorWin", AnimationDone);
      }
      else {
        _CurrentRobot.SendAnimation("shocked", AnimationDone);
      }
    }

    public override void Update() {
      base.Update();
    }

    void AnimationDone(bool success) {
      ((VortexGame)_StateMachine.GetGame()).HandleSpinResultsCompleted();
    }

    public override void Exit() {
      base.Exit();
    }
      
  }

}
