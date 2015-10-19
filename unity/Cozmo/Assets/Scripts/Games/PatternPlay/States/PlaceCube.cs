using UnityEngine;
using System.Collections;

public class PlaceCube : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();

    Vector3 placeTarget = patternPlayAutoBuild.FindPlaceTarget();

    robot.PlaceObjectOnGround(placeTarget, 0.0f, false, false, PlaceDone);
  }

  void PlaceDone(bool success) {
    if (success) {
      patternPlayAutoBuild.PlaceBlockSuccess();
      stateMachine.SetNextState(new LookAtPatternConstruction());
    }
    else {
      stateMachine.SetNextState(new LookForCubes());
    }
  }
}
