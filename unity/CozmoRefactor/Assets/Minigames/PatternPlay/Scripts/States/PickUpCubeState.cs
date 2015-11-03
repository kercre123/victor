using UnityEngine;
using System.Collections;

public class PickUpCubeState : State {

  private PatternPlayGame patternPlayGame_ = null;
  private PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  bool hasTarget = true;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "PickUpCube");
    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGame_.GetAutoBuild();

    ObservedObject targetObject = patternPlayAutoBuild_.GetClosestAvailableBlock();

    // it is possible that since we thought there were seenBlocks that they
    // have been dirtied since then. If there are no available blocks
    // anymore then we should go look for more.
    if (targetObject == null) {
      hasTarget = false;
      return;
    }

    robot.PickupObject(targetObject, true, false, false, 0.0f, PickUpDone);
    patternPlayAutoBuild_.SetObjectHeld(targetObject);
  }

  public override void Update() {
    base.Update();
    if (hasTarget == false) {
      DAS.Info("PatternPlayPickUpCube", "No Cubes To Pickup");
      stateMachine_.SetNextState(new LookForCubesState());
      patternPlayAutoBuild_.SetObjectHeld(null);
    }
    if (patternPlayGame_.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
      return;
    }
  }

  void PickUpDone(bool success) {
    if (success) {
      stateMachine_.SetNextState(new PlaceCubeState());
    }
    else {
      DAS.Info("PatternPlay", "PickUp Failed!");
      stateMachine_.SetNextState(new LookForCubesState());
      if (patternPlayAutoBuild_.GetHeldObject() != null) {
        robot.UpdateDirtyList(patternPlayAutoBuild_.GetHeldObject());
      }

      patternPlayAutoBuild_.SetObjectHeld(null);
    }

  }

}
