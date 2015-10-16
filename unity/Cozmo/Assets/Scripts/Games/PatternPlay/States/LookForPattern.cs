using UnityEngine;
using System.Collections;

public class LookForPattern : State {

  PatternPlayController patternPlayController = null;

  public override void Enter() {
    base.Enter();
    DAS.Info("State", "LookForPattern");
    // TODO: Set eyes to scan.
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
  }

  int lastFrameVisibleCount = 0;

  public override void Update() {
    base.Update();

    float closestD = ClosestsObject();

    if (robot.markersVisibleObjects.Count > 0) {
      if (closestD < 140.0f) {
        robot.DriveWheels(-60.0f, -60.0f);
      }
      else if (closestD > 190.0f) {
        robot.DriveWheels(40.0f, 40.0f);
      }
      else {
        robot.DriveWheels(0.0f, 0.0f);
      }
    }

    // last frame visible count flag is used to prevent locking up the
    // wheels on the idle animation.
    if (lastFrameVisibleCount > 0 && robot.markersVisibleObjects.Count == 0) {
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (patternPlayController.SeenPattern()) {
      stateMachine.SetNextState(new CelebratePattern());
    }
    else if (NoBlocksMoved()) {
      // nobody has moved blocks for a while... ima make my own pattern.
      // stateMachine.SetNextState(new HaveIdeaForPattern());
    }

    lastFrameVisibleCount = robot.markersVisibleObjects.Count;
  }

  bool NoBlocksMoved() {
    return Time.time - patternPlayController.GetMostRecentMovedTime() > 5.0f;
  }

  float ClosestsObject() {
    float closest = float.MaxValue;
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      float d = Vector3.Distance(robot.WorldPosition, robot.markersVisibleObjects[i].WorldPosition);
      if (d < closest) {
        closest = d;
      }
    }
    return closest;
  }
}
