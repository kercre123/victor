using UnityEngine;
using System.Collections;

public class HaveIdeaForPattern : State {

  PatternPlayController patternPlayController = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "HaveIdeaForPattern");

    // pick a pattern to build
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayController.GetAutoBuild().PickNewTargetPattern();

    // TODO: Use "have idea" animation
    robot.SendAnimation("shocked", AnimationDone);
  }

  private void AnimationDone(bool success) {
    stateMachine.SetNextState(new LookForCubes());
  }
}
