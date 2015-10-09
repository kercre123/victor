using UnityEngine;
using System.Collections;

public struct BlockLights {
  public bool front;
  public bool back;
  public bool left;
  public bool right;

  public bool LightsOff() {
    return !front && !back && !left && !right;
  }

  public int NumberOfLightsOn() {
    int count = 0;
    if (front)
      count++;
    if (back)
      count++;
    if (left)
      count++;
    if (right)
      count++;
    return count;
  }

  static public BlockLights GetNextConfig(BlockLights currentConfig) {
    BlockLights newLights = new BlockLights();
    if (!currentConfig.front && !currentConfig.right && !currentConfig.back && !currentConfig.left) {
      newLights.front = true;
    }
    else if (currentConfig.front && !currentConfig.right && !currentConfig.back && !currentConfig.left) {
      newLights.front = true;
      newLights.right = true;
    }
    else if (currentConfig.front && currentConfig.right && !currentConfig.back && !currentConfig.left) {
      newLights.front = true;
      newLights.right = true;
      newLights.back = true;
    }
    else if (currentConfig.front && currentConfig.right && currentConfig.back && !currentConfig.left) {
      newLights.front = true;
      newLights.right = true;
      newLights.back = true;
      newLights.left = true;
    }
    else if (currentConfig.front && currentConfig.right && currentConfig.back && currentConfig.left) {

    }
    return newLights;
  }

  static public BlockLights GetRotatedClockwise(BlockLights currentConfig) {
    BlockLights newLights = new BlockLights();
    newLights.left = currentConfig.front;
    newLights.back = currentConfig.left;
    newLights.right = currentConfig.back;
    newLights.front = currentConfig.right;
    return newLights;
  }
}
