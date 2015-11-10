using UnityEngine;
using System.Collections;

public class LookForPatternState : State {
  private PatternPlayGame _PatternPlayGameRef = null;
  private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  private bool _AnimationPlaying = false;

  private int _LastFrameVisibleCount = 0;
  private int _LastSeenThresholdCount = 0;

  private bool _LastFrameHasVerticalBlock = false;

  public override void Enter() { 
    base.Enter();

    // TODO: Set eyes to scan. 
    _PatternPlayGameRef = (PatternPlayGame)stateMachine_.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGameRef.GetAutoBuild();
    _PatternPlayAutoBuild.autoBuilding = false;
  }

  public override void Update() {
    base.Update(); 

    if (_AnimationPlaying) { 
      return;
    }

    CheckShouldPlayIdle();

    float closestD = ClosestsObject();
    int seenThreshold = _PatternPlayGameRef.SeenBlocksOverThreshold(0.5f);

    bool hasVerticalBlock = _PatternPlayGameRef.HasVerticalStack();

    if (seenThreshold > _LastSeenThresholdCount) {
      robot.SendAnimation("enjoyLight", AnimationDone);
      _AnimationPlaying = true;
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (hasVerticalBlock && !_LastFrameHasVerticalBlock) {
      robot.SetHeadAngle(0.3f);
    }

    if (!hasVerticalBlock && _LastFrameHasVerticalBlock) {
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
    if (_LastFrameVisibleCount > 0 && robot.VisibleObjects.Count == 0) {
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (_PatternPlayGameRef.SeenPattern()) {
      stateMachine_.SetNextState(new CelebratePatternState());
    }
    else if (ShouldAutoBuildPattern()) {
      // nobody has moved blocks for a while... ima make my own pattern.
      //stateMachine.SetNextState(new HaveIdeaForPatternState());
    }

    _LastFrameVisibleCount = robot.VisibleObjects.Count;
    _LastSeenThresholdCount = seenThreshold;
    _LastFrameHasVerticalBlock = hasVerticalBlock;
  }

  bool ShouldAutoBuildPattern() {
    bool blocksNotTouched = Time.time - _PatternPlayGameRef.GetMostRecentMovedTime() > 10.0f;
    bool patternNotSeen = Time.time - _PatternPlayGameRef.LastPatternSeenTime() > 20.0f;
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
      if (_PatternPlayGameRef.GetBlockPatternData(robot.VisibleObjects[i].ID).BlockLightsLocalSpace.AreLightsOff() == false) {
        robot.SetIdleAnimation("NONE");
        return;
      }
    }

    robot.SetIdleAnimation("_LIVE_");
  }

  private void AnimationDone(bool success) {
    _AnimationPlaying = false;
  }
}
