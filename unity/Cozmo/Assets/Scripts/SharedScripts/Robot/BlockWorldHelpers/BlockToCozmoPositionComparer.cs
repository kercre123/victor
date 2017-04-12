using UnityEngine;
using System;
using System.Collections.Generic;

// Sorts a list from left to right based on relative to Cozmo position
public class BlockToCozmoPositionComparer : Comparer<LightCube> {
  private IRobot CurrentRobot;

  public BlockToCozmoPositionComparer(IRobot robot) {
    CurrentRobot = robot;
  }

  public override int Compare(LightCube a, LightCube b) {
    Vector3 cozmoSpaceA = CurrentRobot.WorldToCozmo(a.WorldPosition);
    Vector3 cozmoSpaceB = CurrentRobot.WorldToCozmo(b.WorldPosition);
    if (cozmoSpaceA.y > cozmoSpaceB.y) {
      return 1;
    }
    else if (cozmoSpaceA.y < cozmoSpaceB.y) {
      return -1;
    }
    return 0;
  }
}