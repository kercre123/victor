using UnityEngine;
using System.Collections;

public class HaveIdeaForPattern : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  bool hasTargetToBuild = false;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "HaveIdeaForPattern");

    // pick a pattern to build
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
    patternPlayAutoBuild.autoBuilding = true;

    hasTargetToBuild = patternPlayAutoBuild.PickNewTargetPattern();

    if (hasTargetToBuild == false) {
      DAS.Info("PatternPlayState", "No new patterns to build, returning to look for pattern");
      return;
    }

    // TODO: Use "have idea" animation
    robot.SendAnimation("shocked", AnimationDone);
  }

  public override void Update() {
    base.Update();
    if (!hasTargetToBuild) {
      stateMachine.SetNextState(new LookForPattern());
    }
  }

  private void AnimationDone(bool success) {
    stateMachine.SetNextState(new LookForCubes());
  }
}
