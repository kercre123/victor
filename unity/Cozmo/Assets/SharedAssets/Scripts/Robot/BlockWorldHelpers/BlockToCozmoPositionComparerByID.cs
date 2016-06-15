using UnityEngine;
using System;
using System.Collections.Generic;

// Sorts a list from left to right based on relative to Cozmo position
public class BlockToCozmoPositionComparerByID : Comparer<int> {
  private IRobot CurrentRobot;

  public BlockToCozmoPositionComparerByID(IRobot robot) {
    CurrentRobot = robot;
  }

  public override int Compare(int a_id, int b_id) {
    LightCube a;
    LightCube b;
    if (!CurrentRobot.LightCubes.TryGetValue(a_id, out a) || !CurrentRobot.LightCubes.TryGetValue(b_id, out b)) {
      return 0;
    }
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