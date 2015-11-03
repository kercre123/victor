using UnityEngine;
using System.Collections;

public class SetCubeToPatternState : State {

  private PatternPlayGame patternPlayGame_ = null;
  private PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "SetCubeToPattern");

    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGame_.GetAutoBuild();
  }

  public override void Update() {
    base.Update();
    SetPattern();
    stateMachine_.SetNextState(new LookAtPatternConstructionState());
  }

  void SetPattern() {
    int heldObjectID = patternPlayAutoBuild_.GetHeldObject().ID;
    patternPlayAutoBuild_.SetBlockLightsToPattern(heldObjectID);
    patternPlayAutoBuild_.PlaceBlockSuccess();
  }
}
