using UnityEngine;
using System.Collections;

public class SetCubeToPatternState : State {

  private PatternPlayGame _PatternPlayGame = null;
  private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "SetCubeToPattern");

    _PatternPlayGame = (PatternPlayGame)_StateMachine.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();
  }

  public override void Update() {
    base.Update();
    SetPattern();
    _StateMachine.SetNextState(new LookAtPatternConstructionState());
  }

  void SetPattern() {
    int heldObjectID = _PatternPlayAutoBuild.GetHeldObject().ID;
    _PatternPlayAutoBuild.SetBlockLightsToPattern(heldObjectID);
    _PatternPlayAutoBuild.PlaceBlockSuccess();
  }
}
