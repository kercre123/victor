using UnityEngine;
using System.Collections;

public class SetCubeToPattern : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
    SetPattern();
    stateMachine.SetNextState(new PlaceCube());
  }

  void SetPattern() {
    int heldObjectID = patternPlayAutoBuild.GetHeldObject().ID;
    patternPlayAutoBuild.SetBlockLightsToPattern(heldObjectID);

    // TODO: play some kind of sound
  }
}
