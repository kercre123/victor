using UnityEngine;
using System.Collections;

public class HaveIdeaForPatternState : State {

  PatternPlayGame _PatternPlayGame = null;
  PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  private bool _HasTargetToBuild = false;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "HaveIdeaForPattern");

    // pick a pattern to build
    _PatternPlayGame = (PatternPlayGame)stateMachine_.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();
    _PatternPlayAutoBuild.autoBuilding = true;

    _HasTargetToBuild = _PatternPlayAutoBuild.PickNewTargetPattern();

    if (_HasTargetToBuild == false) {
      DAS.Info("PatternPlayState", "No new patterns to build, returning to look for pattern");
      return;
    }

    // TODO: Use "have idea" animation
    robot.SendAnimation("shocked", AnimationDone);
  }

  public override void Update() {
    base.Update();

    if (_PatternPlayGame.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
      return;
    }

    if (!_HasTargetToBuild) {
      stateMachine_.SetNextState(new LookForPatternState());
    }
  }

  private void AnimationDone(bool success) {
    stateMachine_.SetNextState(new LookForCubesState());
  }
}
