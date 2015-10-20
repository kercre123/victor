using UnityEngine;
using System.Collections;

public class LookForCubes : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "LookForCubes");
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
  }

  public override void Update() {
    base.Update();
    if (patternPlayAutoBuild.AvailableBlocks() > 0) {
      stateMachine.SetNextState(new PickUpCube());
      // robot.ExecuteBehavior("NONE");
    }
    else {
      SearchForAvailableBlock();
    }
  }

  void SearchForAvailableBlock() {
    // robot.ExecuteBehavior("LOOK_AROUND");
  }
}
