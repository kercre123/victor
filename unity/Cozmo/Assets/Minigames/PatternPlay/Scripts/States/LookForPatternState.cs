using UnityEngine;
using System.Collections;

namespace PatternPlay {

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
      _PatternPlayGameRef = (PatternPlayGame)_StateMachine.GetGame();
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
        _CurrentRobot.SendAnimation("enjoyLight", HandleAnimationDone);
        _AnimationPlaying = true;
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }

      if (hasVerticalBlock && !_LastFrameHasVerticalBlock) {
        _CurrentRobot.SetHeadAngle(0.3f);
      }

      if (!hasVerticalBlock && _LastFrameHasVerticalBlock) {
        _CurrentRobot.SetHeadAngle(-0.1f);
      }

      if (_CurrentRobot.VisibleObjects.Count > 0) {
        if (closestD < 140.0f) {
          _CurrentRobot.DriveWheels(-60.0f, -60.0f);
        }
        else if (closestD > 190.0f) {
          _CurrentRobot.DriveWheels(40.0f, 40.0f);
        }
        else {
          _CurrentRobot.DriveWheels(0.0f, 0.0f);
        }
      }

      // last frame visible count flag is used to prevent locking up the
      // wheels on the idle animation.
      if (_LastFrameVisibleCount > 0 && _CurrentRobot.VisibleObjects.Count == 0) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }

      if (_PatternPlayGameRef.SeenPattern()) {
        _StateMachine.SetNextState(new CelebratePatternState());
      }
      else if (ShouldAutoBuildPattern()) {
        // nobody has moved blocks for a while... ima make my own pattern.
        //stateMachine.SetNextState(new HaveIdeaForPatternState());
      }

      _LastFrameVisibleCount = _CurrentRobot.VisibleObjects.Count;
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
      _CurrentRobot.SetIdleAnimation("NONE");
    }

    float ClosestsObject() {
      float closest = float.MaxValue;
      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        float d = Vector3.Distance(_CurrentRobot.WorldPosition, _CurrentRobot.VisibleObjects[i].WorldPosition);
        if (d < closest) {
          closest = d;
        }
      }
      return closest;
    }

    private void CheckShouldPlayIdle() {
      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        if (_PatternPlayGameRef.GetBlockPatternData(_CurrentRobot.VisibleObjects[i].ID).BlockLightsLocalSpace.AreLightsOff() == false) {
          _CurrentRobot.SetIdleAnimation("NONE");
          return;
        }
      }

      _CurrentRobot.SetIdleAnimation("_LIVE_");
    }

    private void HandleAnimationDone(bool success) {
      _AnimationPlaying = false;
    }
  }

}
