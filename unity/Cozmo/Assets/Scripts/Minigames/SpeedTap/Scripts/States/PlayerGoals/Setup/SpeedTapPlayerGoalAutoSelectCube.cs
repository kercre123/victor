using UnityEngine;
using System.Collections.Generic;

namespace SpeedTap {
  public class SpeedTapPlayerGoalAutoSelectCube : SpeedTapPlayerGoalBaseSelectCube {

    private List<int> _AlreadyTapped = new List<int>();
    // Waits for Tap and then completes...
    public override void Enter() {
      base.Enter();

    }
    public override void Exit() {
      base.Exit();

    }

    public override void Update() {
      base.Update();
      // Tap a different cube every frame.
      int id = _BlockID;
      if (_BlockID == -1) {
        // Try tapping on a cube a frame
        foreach (var kvp in _CurrentRobot.LightCubes) {
          if (!_AlreadyTapped.Contains(kvp.Key)) {
            id = kvp.Key;
            _AlreadyTapped.Add(id);
            break;
          }
        }
      }
      if (OnCubeSelected != null) {
        OnCubeSelected(_ParentPlayer, id);
      }
    }


  }
}