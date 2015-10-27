using UnityEngine;
using System.Collections;

public class PlaceCube : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "PlaceCube");

    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();

    Vector3 placeTarget = patternPlayAutoBuild.FindPlaceTarget();

    robot.PlaceObjectOnGround(placeTarget, Quaternion.identity, false, false, PlaceDone);
  }

  void PlaceDone(bool success) {
    if (success) {
      patternPlayAutoBuild.PlaceBlockSuccess();
      stateMachine.SetNextState(new LookAtPatternConstruction());
    }
    else {
      robot.PlaceObjectOnGroundHere(PlaceGroundHere);
    }
  }

  void PlaceGroundHere(bool success) {
    stateMachine.SetNextState(new LookForCubes());
  }
}
