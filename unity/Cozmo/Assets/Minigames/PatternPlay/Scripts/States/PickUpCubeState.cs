using UnityEngine;
using System.Collections;

public class PickUpCubeState : State {

  private PatternPlayGame _PatternPlayGame = null;
  private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  private bool _HasTarget = true;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "PickUpCube");
    _PatternPlayGame = (PatternPlayGame)_StateMachine.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();

    ObservedObject targetObject = _PatternPlayAutoBuild.GetClosestAvailableBlock();

    // it is possible that since we thought there were seenBlocks that they
    // have been dirtied since then. If there are no available blocks
    // anymore then we should go look for more.
    if (targetObject == null) {
      _HasTarget = false;
      return;
    }

    _CurrentRobot.PickupObject(targetObject, true, false, false, 0.0f, PickUpDone);
    _PatternPlayAutoBuild.SetObjectHeld(targetObject);
  }

  public override void Update() {
    base.Update();
    if (_HasTarget == false) {
      DAS.Info("PatternPlayPickUpCube", "No Cubes To Pickup");
      _StateMachine.SetNextState(new LookForCubesState());
      _PatternPlayAutoBuild.SetObjectHeld(null);
    }
    if (_PatternPlayGame.SeenPattern()) {
      _StateMachine.SetNextState(new CelebratePatternState());
      return;
    }
  }

  void PickUpDone(bool success) {
    if (success) {
      _StateMachine.SetNextState(new PlaceCubeState());
    }
    else {
      DAS.Info("PatternPlay", "PickUp Failed!");
      _StateMachine.SetNextState(new LookForCubesState());
      if (_PatternPlayAutoBuild.GetHeldObject() != null) {
        _CurrentRobot.UpdateDirtyList(_PatternPlayAutoBuild.GetHeldObject());
      }

      _PatternPlayAutoBuild.SetObjectHeld(null);
    }

  }

}
