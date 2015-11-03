using UnityEngine;
using System.Collections;

public class LookForPatternState : State {
  PatternPlayGame patternPlayGameRef_ = null;
  PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  bool animationPlaying_ = false;

  int lastFrameVisibleCount_ = 0;
  int lastSeenThresholdCount_ = 0;

  bool lastFrameHasVerticalBlock_ = false;

  public override void Enter() { 
    base.Enter();

    // TODO: Set eyes to scan. 
    patternPlayGameRef_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGameRef_.GetAutoBuild();
    patternPlayAutoBuild_.autoBuilding = false;
  }

  public override void Update() {
    base.Update(); 

    if (animationPlaying_) { 
      return;
    }

    CheckShouldPlayIdle();

    float closestD = ClosestsObject();
    int seenThreshold = patternPlayGameRef_.SeenBlocksOverThreshold(0.5f);

    bool hasVerticalBlock = patternPlayGameRef_.HasVerticalStack();

    if (seenThreshold > lastSeenThresholdCount_) {
      robot.SendAnimation("enjoyLight", AnimationDone);
      animationPlaying_ = true;
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (hasVerticalBlock && !lastFrameHasVerticalBlock_) {
      robot.SetHeadAngle(0.3f);
    }

    if (!hasVerticalBlock && lastFrameHasVerticalBlock_) {
      robot.SetHeadAngle(-0.1f);
    }

    if (robot.VisibleObjects.Count > 0) {
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
    if (lastFrameVisibleCount_ > 0 && robot.VisibleObjects.Count == 0) {
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (patternPlayGameRef_.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
    }
    else if (ShouldAutoBuildPattern()) {
      // nobody has moved blocks for a while... ima make my own pattern.
      //stateMachine.SetNextState(new HaveIdeaForPattern());
    }

    lastFrameVisibleCount_ = robot.VisibleObjects.Count;
    lastSeenThresholdCount_ = seenThreshold;
    lastFrameHasVerticalBlock_ = hasVerticalBlock;
  }

  bool ShouldAutoBuildPattern() {
    bool blocksNotTouched = Time.time - patternPlayGameRef_.GetMostRecentMovedTime() > 10.0f;
    bool patternNotSeen = Time.time - patternPlayGameRef_.LastPatternSeenTime() > 20.0f;
    return blocksNotTouched && patternNotSeen;
  }

  public override void Exit() {
    base.Exit();
    robot.SetIdleAnimation("NONE");
  }

  float ClosestsObject() {
    float closest = float.MaxValue;
    for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
      float d = Vector3.Distance(robot.WorldPosition, robot.VisibleObjects[i].WorldPosition);
      if (d < closest) {
        closest = d;
      }
    }
    return closest;
  }

  private void CheckShouldPlayIdle() {
    for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
      if (patternPlayGameRef_.GetBlockPatternData(robot.VisibleObjects[i].ID).blockLightsLocalSpace.AreLightsOff() == false) {
        robot.SetIdleAnimation("NONE");
        return;
      }
    }

    robot.SetIdleAnimation("_LIVE_");
  }

  private void AnimationDone(bool success) {
    animationPlaying_ = false;
  }
}
