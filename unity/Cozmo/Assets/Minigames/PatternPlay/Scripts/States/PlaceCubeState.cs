using UnityEngine;
using System.Collections;

public class PlaceCubeState : State {

  private PatternPlayGame _PatternPlayGame = null;
  private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "PlaceCube");

    _PatternPlayGame = (PatternPlayGame)stateMachine_.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();

    Vector3 placeTarget;
    int dockID;
    float offset;
    float dockAngleRads;

    _PatternPlayAutoBuild.FindPlaceTarget(out placeTarget, out dockID, out offset, out dockAngleRads);
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
