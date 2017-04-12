using UnityEngine;
using System.Collections.Generic;

namespace SpeedTap {
  public class SpeedTapPlayerGoalTapAuto : PlayerGoalBase {

    private const float _kWaitTime_Sec = 0.4f;
    private float _StartTime = 0;
    private bool _ShouldTap;
    // Waits for Tap and then nothing.
    // Gamestate will close us.
    public SpeedTapPlayerGoalTapAuto(bool shouldTap) {
      _ShouldTap = shouldTap;
    }

    public override void Enter() {
      base.Enter();
      _StartTime = Time.time;
    }
    public override void Exit() {
      base.Exit();

    }

    public override void Update() {
      base.Update();
      if (_ShouldTap) {
        SpeedTapPlayerInfo speedTapPlayerInfo = (SpeedTapPlayerInfo)_ParentPlayer;
        // Taps before animation would even stream down
        if ((Time.time - _StartTime) > _kWaitTime_Sec && speedTapPlayerInfo.LastTapTimeStamp < 0) {
          // since we just check lowest number, this will always be first
          speedTapPlayerInfo.LastTapTimeStamp = 1;
        }
      }
    }

  }
}