using UnityEngine;
using System.Collections;

public class LookForPattern : State {

  PatternPlayController patternPlayController = null;

  bool animationPlaying = false;

  public override void Enter() {
    base.Enter();
    DAS.Info("State", "LookForPattern");
    // TODO: Set eyes to scan.
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
  }

  int lastFrameVisibleCount = 0;
  int lastSeenThresholdCount = 0;

  bool lastFrameHasVerticalBlock = false;

  public override void Update() {
    base.Update();

    if (animationPlaying) {
      return;
    }

    float closestD = ClosestsObject();
    int seenThreshold = patternPlayController.SeenBlocksOverThreshold(0.5f);

    bool hasVerticalBlock = patternPlayController.HasVerticalBlock();

    if (seenThreshold > lastSeenThresholdCount) {
      robot.SendAnimation("enjoyLight", AnimationDone);
      animationPlaying = true;
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (hasVerticalBlock && !lastFrameHasVerticalBlock) {
      robot.SetHeadAngle(0.2f);
    }

    if (!hasVerticalBlock && lastFrameHasVerticalBlock) {
      robot.SetHeadAngle(-0.1f);
    }

    if (robot.visibleObjects.Count > 0) {
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
    if (lastFrameVisibleCount > 0 && robot.visibleObjects.Count == 0) {
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (patternPlayController.SeenPattern()) {
      stateMachine.SetNextState(new CelebratePattern());
    }
    else if (NoBlocksMoved()) {
      // nobody has moved blocks for a while... ima make my own pattern.
      // stateMachine.SetNextState(new HaveIdeaForPattern());
    }

    lastFrameVisibleCount = robot.visibleObjects.Count;
    lastSeenThresholdCount = seenThreshold;
    lastFrameHasVerticalBlock = hasVerticalBlock;
  }

  bool NoBlocksMoved() {
    return Time.time - patternPlayController.GetMostRecentMovedTime() > 5.0f;
  }

  float ClosestsObject() {
    float closest = float.MaxValue;
    for (int i = 0; i < robot.visibleObjects.Count; ++i) {
      float d = Vector3.Distance(robot.WorldPosition, robot.visibleObjects[i].WorldPosition);
      if (d < closest) {
        closest = d;
      }
    }
    return closest;
  }

  void AnimationDone(bool success) {
    animationPlaying = false;
  }
}
