using UnityEngine;
using System.Collections;

public class LookForCubesState : State {

  private PatternPlayGame patternPlayGame_ = null;
  private PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  bool lookingAround = false;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "LookForCubes");
    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGame_.GetAutoBuild();
  }

  public override void Update() {
    base.Update();

    if (patternPlayGame_.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
      return;
    }

    if (patternPlayAutoBuild_.AvailableBlocks() > 0) {
      stateMachine_.SetNextState(new PickUpCubeState());
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
