using UnityEngine;
using System.Collections;

namespace DockTraining {

  public class CelebrateState : State {
    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation("majorWin", AnimationDone);
    }

    private void AnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForTargetState());
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
    }
  }

}
