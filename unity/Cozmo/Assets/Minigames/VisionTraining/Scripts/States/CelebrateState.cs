using UnityEngine;
using System.Collections;

namespace VisionTraining {
  public class CelebrateState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation(AnimationName.kMajorWin, HandleAnimationDone);
    }

    void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new RecognizeCubeState());
    }

    public override void Exit() {
      base.Exit();
    }
  }

}

