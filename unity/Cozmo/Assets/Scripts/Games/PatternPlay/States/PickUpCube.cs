using UnityEngine;
using System.Collections;

public class PickUpCube : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "PickUpCube");
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();

    ObservedObject targetObject = patternPlayAutoBuild.GetClosestAvailableBlock();

    // it is possible that since we thought there were seenBlocks that they
    // have been dirtied since then. If there are no available blocks
    // anymore then we should go look for more.
    if (targetObject == null) {
      stateMachine.SetNextState(new LookForCubes());
      patternPlayAutoBuild.SetObjectHeld(null);
      return;
    }

    robot.PickAndPlaceObject(targetObject, true, false, PickUpDone);
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
