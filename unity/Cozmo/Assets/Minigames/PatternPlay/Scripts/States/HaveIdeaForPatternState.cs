using UnityEngine;
using System.Collections;

namespace PatternPlay {

  public class HaveIdeaForPatternState : State {

    PatternPlayGame _PatternPlayGame = null;
    PatternPlayAutoBuild _PatternPlayAutoBuild = null;

    private bool _HasTargetToBuild = false;

    public override void Enter() {
      base.Enter();

      DAS.Info(this, "HaveIdeaForPattern");

      // pick a pattern to build
      _PatternPlayGame = (PatternPlayGame)_StateMachine.GetGame();
      _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();
      _PatternPlayAutoBuild.autoBuilding = true;

      _HasTargetToBuild = _PatternPlayAutoBuild.PickNewTargetPattern();

      if (_HasTargetToBuild == false) {
        DAS.Info(this, "No new patterns to build, returning to look for pattern");
        return;
      }

      // TODO: Use "have idea" animation
      _CurrentRobot.SendAnimation("shocked", AnimationDone);
    }

    public override void Update() {
      base.Update();

      if (_PatternPlayGame.SeenPattern()) {
        _StateMachine.SetNextState(new CelebratePatternState());
        return;
      }

      if (!_HasTargetToBuild) {
        _StateMachine.SetNextState(new LookForPatternState());
      }
    }

    private void AnimationDone(bool success) {
      _StateMachine.SetNextState(new LookForCubesState());
    }
  }

}
