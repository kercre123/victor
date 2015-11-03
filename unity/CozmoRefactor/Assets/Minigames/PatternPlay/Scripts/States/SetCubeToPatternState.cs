using UnityEngine;
using System.Collections;

public class SetCubeToPatternState : State {

  PatternPlayGame patternPlayGame_ = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "SetCubeToPattern");

    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild = patternPlayGame_.GetAutoBuild();
  }

  public override void Update() {
    base.Update();
    SetPattern();
    stateMachine_.SetNextState(new LookAtPatternConstructionState());
  }

  void SetPattern() {
    int heldObjectID = patternPlayAutoBuild.GetHeldObject().ID;
    patternPlayAutoBuild.SetBlockLightsToPattern(heldObjectID);
    patternPlayAutoBuild.PlaceBlockSuccess();
  }
}
