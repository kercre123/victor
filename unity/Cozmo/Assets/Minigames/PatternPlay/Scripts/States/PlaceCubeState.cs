using UnityEngine;
using System.Collections;

namespace PatternPlay {

  public class PlaceCubeState : State {

    private PatternPlayGame _PatternPlayGame = null;
    private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

    public override void Enter() {
      base.Enter();

      DAS.Info("PatternPlayState", "PlaceCube");

      _PatternPlayGame = (PatternPlayGame)_StateMachine.GetGame();
      _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();

      Vector3 placeTarget;
      int dockID;
      float offset;
      float dockAngleRads;

      _PatternPlayAutoBuild.FindPlaceTarget(out placeTarget, out dockID, out offset, out dockAngleRads);
      if (dockID == -1) {
        _CurrentRobot.PlaceObjectOnGround(placeTarget, Quaternion.identity, false, false, PlaceDone);
      }
      else {
        _CurrentRobot.PlaceObjectRel(_CurrentRobot.LightCubes[dockID], offset, dockAngleRads, PlaceDone);
      }
    }

    void PlaceDone(bool success) {
      if (success) {
        _StateMachine.SetNextState(new SetCubeToPatternState());
      }
      else {
        _CurrentRobot.PlaceObjectOnGroundHere(PlaceGroundHere);
      }
    }

    void PlaceGroundHere(bool success) {
      _StateMachine.SetNextState(new LookForCubesState());
    }
  }

}
