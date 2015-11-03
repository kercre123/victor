using UnityEngine;
using System.Collections;

public class PlaceCubeState : State {

  PatternPlayGame patternPlayGame_ = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "PlaceCube");

    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild = patternPlayGame_.GetAutoBuild();

    Vector3 placeTarget;
    int dockID;
    float offset;
    float dockAngleRads;

    patternPlayAutoBuild.FindPlaceTarget(out placeTarget, out dockID, out offset, out dockAngleRads);
    if (dockID == -1) {
      robot.PlaceObjectOnGround(placeTarget, Quaternion.identity, false, false, PlaceDone);
    }
    else {
      robot.PlaceObjectRel(robot.ActiveBlocks[dockID], offset, dockAngleRads, PlaceDone);
    }
  }

  void PlaceDone(bool success) {
    if (success) {
      stateMachine_.SetNextState(new SetCubeToPatternState());
    }
    else {
      Debug.LogError("place failed");
      robot.PlaceObjectOnGroundHere(PlaceGroundHere);
    }
  }

  void PlaceGroundHere(bool success) {
    stateMachine_.SetNextState(new LookForCubesState());
  }
}
