using UnityEngine;
using System.Collections;

public class LookForCubesState : State {

  private PatternPlayGame _PatternPlayGame = null;
  private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  private bool lookingAround = false;

  public override void Enter() {
    base.Enter();
    DAS.Info("PatternPlayState", "LookForCubes");
    _PatternPlayGame = (PatternPlayGame)stateMachine_.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();
  }

  public override void Update() {
    base.Update();

    if (_PatternPlayGame.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
      return;
    }

    if (_PatternPlayAutoBuild.AvailableBlocks() > 0) {
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
