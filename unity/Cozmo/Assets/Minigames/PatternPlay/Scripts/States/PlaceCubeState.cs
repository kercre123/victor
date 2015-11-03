using UnityEngine;
using System.Collections;

public class PlaceCubeState : State {

  private PatternPlayGame patternPlayGame_ = null;
  private PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "PlaceCube");

    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGame_.GetAutoBuild();

    Vector3 placeTarget;
    int dockID;
    float offset;
    float dockAngleRads;

    patternPlayAutoBuild_.FindPlaceTarget(out placeTarget, out dockID, out offset, out dockAngleRads);
    if (dockID == -1) {
      robot.PlaceObjectOnGround(placeTarget, Quaternion.identity, false, false, PlaceDone);
    }
    else {
      robot.PlaceObjectRel(robot.LightCubes[dockID], offset, dockAngleRads, PlaceDone);
    }
  }

  void PlaceDone(bool success) {
    if (success) {
      stateMachine_.SetNextState(new SetCubeToPatternState());
    }
    else {
      robot.PlaceObjectOnGroundHere(PlaceGroundHere);
    }
  }

  void PlaceGroundHere(bool success) {
    stateMachine_.SetNextState(new LookForCubesState());
  }
}
