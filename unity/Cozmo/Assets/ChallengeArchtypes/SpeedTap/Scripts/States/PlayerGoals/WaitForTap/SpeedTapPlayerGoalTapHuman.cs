using UnityEngine;
using System.Collections.Generic;

namespace SpeedTap {
  public class SpeedTapPlayerGoalTapHuman : PlayerGoalBase {

    // Waits for Tap and then nothing.
    // Gamestate will close us.
    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += HandleTap;
    }
    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= HandleTap;
    }

    // This PlayerGoal can only be completed from the gamestate.
    // It doesn't complete itself.
    private void HandleTap(int id, int tappedTimes, float timeStamp) {
      SpeedTapPlayerInfo speedTapPlayerInfo = (SpeedTapPlayerInfo)_ParentPlayer;
      if (id == speedTapPlayerInfo.CubeID) {
        if (timeStamp < speedTapPlayerInfo.LastTapTimeStamp || speedTapPlayerInfo.LastTapTimeStamp < 0) {
          speedTapPlayerInfo.LastTapTimeStamp = timeStamp;
        }
      }
    }

  }
}