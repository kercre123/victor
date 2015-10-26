using UnityEngine;
using System.Collections;

public class LookForCubes : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  bool lookingAround = false;

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
    robot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    lookingAround = false;
  }

  void SearchForAvailableBlock() {
    if (lookingAround == false) {
      lookingAround = true;
      robot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
    }
  }
}
