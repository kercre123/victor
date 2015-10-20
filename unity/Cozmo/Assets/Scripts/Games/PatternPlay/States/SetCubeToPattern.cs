using UnityEngine;
using System.Collections;

public class SetCubeToPattern : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "SetCubeToPattern");

    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
  }

  public override void Update() {
    base.Update();
    SetPattern();
    stateMachine.SetNextState(new PlaceCube());
  }

  void SetPattern() {
    int heldObjectID = patternPlayAutoBuild.GetHeldObject().ID;
    patternPlayAutoBuild.SetBlockLightsToPattern(heldObjectID);
  }
}
