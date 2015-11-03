using UnityEngine;
using System.Collections;

public class HaveIdeaForPatternState : State {

  PatternPlayGame patternPlayGame_ = null;
  PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  bool hasTargetToBuild = false;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "HaveIdeaForPattern");

    // pick a pattern to build
    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGame_.GetAutoBuild();
    patternPlayAutoBuild_.autoBuilding = true;

    hasTargetToBuild = patternPlayAutoBuild_.PickNewTargetPattern();

    if (hasTargetToBuild == false) {
      DAS.Info("PatternPlayState", "No new patterns to build, returning to look for pattern");
      return;
    }

    // TODO: Use "have idea" animation
    robot.SendAnimation("shocked", AnimationDone);
  }

  public override void Update() {
    base.Update();

    if (patternPlayGame_.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
      return;
    }

    if (!hasTargetToBuild) {
      stateMachine_.SetNextState(new LookForPatternState());
    }
  }

  private void AnimationDone(bool success) {
    stateMachine_.SetNextState(new LookForCubesState());
  }
}
