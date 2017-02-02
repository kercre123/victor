using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapPlayerGoalHumanSelectCube : SpeedTapPlayerGoalBaseSelectCube {

    // Waits for Tap and then completes...
    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += HandleTap;
    }
    // Waits for Tap and then completes...
    public override void Exit() {
      base.Exit();

      LightCube.TappedAction -= HandleTap;
    }

    // This PlayerGoal can only be completed from the gamestate.
    // It doesn't complete itself.
    private void HandleTap(int id, int tappedTimes, float timeStamp) {
      if (OnCubeSelected != null) {
        OnCubeSelected(_ParentPlayer, id);
      }
    }
  }
}