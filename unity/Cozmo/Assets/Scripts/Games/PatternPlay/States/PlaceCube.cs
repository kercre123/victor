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

    Vector3 placeTarget;
    int dockID;
    float offset;
    float dockAngleRads;

    patternPlayAutoBuild.FindPlaceTarget(out placeTarget, out dockID, out offset, out dockAngleRads);
    if (dockID == -1) {
      robot.PlaceObjectOnGround(placeTarget, Quaternion.identity, false, false, PlaceDone);
    }
    else {
      robot.PlaceObjectRel(robot.activeBlocks[dockID], offset, dockAngleRads, PlaceDone);
    }
  }

  void PlaceDone(bool success) {
    if (success) {
      patternPlayAutoBuild.PlaceBlockSuccess();
      stateMachine.SetNextState(new LookAtPatternConstruction());
    }
    else {
      Debug.LogError("place failed");
      robot.PlaceObjectOnGroundHere(PlaceGroundHere);
    }
  }

  void PlaceGroundHere(bool success) {
    stateMachine.SetNextState(new LookForCubes());
  }
}
