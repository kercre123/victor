using UnityEngine;
using System.Collections;

namespace Vortex {

  public class StateSpinComplete : State {

    //private VortexGame _VortexGame = null;


    public override void Enter() {
      base.Enter();
      DAS.Info("StateSpinComplete2", "StateSpinComplete2");
      //_VortexGame = (VortexGame)_StateMachine.GetGame();
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
