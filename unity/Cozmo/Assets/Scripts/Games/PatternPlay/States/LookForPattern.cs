using UnityEngine;
using System.Collections;

public class LookForPattern : State {

  PatternPlayController patternPlayController = null;

  public override void Enter() {
    base.Enter();
    // TODO: Set eyes to scan.
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
  }

  public override void Update() {
    base.Update();
    if (patternPlayController.SeenPattern()) {
      stateMachine.SetNextState(new CelebratePattern());
    }
    else if (NoBlocksMoved()) {
      // nobody has moved blocks for a while... ima make my own pattern.
      stateMachine.SetNextState(new HaveIdeaForPattern());
    }
  }

  bool NoBlocksMoved() {
    return Time.time - patternPlayController.GetMostRecentMovedTime() > 5.0f;
  }
}
