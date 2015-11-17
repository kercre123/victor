using UnityEngine;
using System.Collections;

namespace Vortex {

  public class StateOutro : State {

    public override void Enter() {
      base.Enter();
      DAS.Info(this, "StateOutro");
    }

    public override void Update() {
      base.Update();

    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    }

    void SearchForAvailableBlock() {
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
    }
  }

}
