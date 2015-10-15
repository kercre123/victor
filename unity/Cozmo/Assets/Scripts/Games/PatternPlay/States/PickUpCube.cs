using UnityEngine;
using System.Collections;

public class PickUpCube : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
    ObservedObject targetObject = patternPlayAutoBuild.GetClosestAvailableBlock();

    robot.PickAndPlaceObject(targetObject);
    patternPlayAutoBuild.SetObjectHeld(targetObject);
  }

  void PickUpDone(bool success) {
    if (success) {
      stateMachine.SetNextState(new SetCubeToPattern());
    }
    else {
      stateMachine.SetNextState(new LookForCubes());
      patternPlayAutoBuild.SetObjectHeld(null);
    }

  }

}
