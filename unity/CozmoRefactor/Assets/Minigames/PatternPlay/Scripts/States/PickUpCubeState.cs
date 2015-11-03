using UnityEngine;
using System.Collections;

public class PickUpCubeState : State {

  PatternPlayGame patternPlayGame_ = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  bool hasTarget = true;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "PickUpCube");
    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild = patternPlayGame_.GetAutoBuild();

    ObservedObject targetObject = patternPlayAutoBuild.GetClosestAvailableBlock();

    // it is possible that since we thought there were seenBlocks that they
    // have been dirtied since then. If there are no available blocks
    // anymore then we should go look for more.
    if (targetObject == null) {
      hasTarget = false;
      return;
    }

    robot.PickupObject(targetObject, true, false, false, 0.0f, PickUpDone);
    patternPlayAutoBuild.SetObjectHeld(targetObject);
  }

  public override void Update() {
    base.Update();
    if (hasTarget == false) {
      DAS.Info("PatternPlayPickUpCube", "No Cubes To Pickup");
      stateMachine_.SetNextState(new LookForCubesState());
      patternPlayAutoBuild.SetObjectHeld(null);
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
      if (patternPlayAutoBuild.GetHeldObject() != null) {
        robot.UpdateDirtyList(patternPlayAutoBuild.GetHeldObject());
      }

      patternPlayAutoBuild.SetObjectHeld(null);
    }

  }

}
