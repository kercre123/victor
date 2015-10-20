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

    if (patternPlayController.SeenPattern()) {
      stateMachine.SetNextState(new CelebratePattern());
      return;
    }

    if (patternPlayAutoBuild.AvailableBlocks() > 0) {
      stateMachine.SetNextState(new PickUpCube());
    }
    else {
      SearchForAvailableBlock();
    }
  }

  public override void Exit() {
    base.Exit();
    // robot.ExecuteBehavior("NONE");
  }

  void SearchForAvailableBlock() {
    // robot.ExecuteBehavior("LOOK_AROUND");
  }
}
