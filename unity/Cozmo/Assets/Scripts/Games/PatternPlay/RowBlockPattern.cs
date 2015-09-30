using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class RowBlockPattern {

  // block lights in cozmo-space.
  // front is the light facing cozmo.
  // left is the light left of cozmo etc.
  public List<BlockLights> blocks = new List<BlockLights>();

  public override bool Equals(System.Object obj) {
    if (obj == null) {
      return false;
    }

    RowBlockPattern p = obj as RowBlockPattern;
    if ((System.Object)p == null) {
      return false;
    }
    return Equals(p);
  }

  public bool Equals(RowBlockPattern pattern) {
    if (pattern == null)
      return false;

    if (pattern.blocks.Count != blocks.Count)
      return false;

    for (int i = 0; i < pattern.blocks.Count; ++i) {
      if (pattern.blocks[i].back != blocks[i].back ||
          pattern.blocks[i].front != blocks[i].front ||
          pattern.blocks[i].left != blocks[i].left ||
          pattern.blocks[i].right != blocks[i].right) {
        return false;
      }
    }
    return true;
  }

  public override int GetHashCode() {
    int x = 0;
    for (int i = 0; i < blocks.Count; ++i) {
      x ^= System.Convert.ToInt32(blocks[i].back) ^ System.Convert.ToInt32(blocks[i].front) ^
      System.Convert.ToInt32(blocks[i].left) ^ System.Convert.ToInt32(blocks[i].right);
    }
    return x;
  }

  static public void SetRandomConfig(Robot robot, Dictionary<int, BlockLights> blockLights, RowBlockPattern lastPatternSeen) {
    BlockLights patternLight = lastPatternSeen.blocks[0];

    // generate a new config that is not the current config.
    int nextConfigCount = Random.Range(1, 5);
    BlockLights newBlockLight = patternLight;
    while (nextConfigCount > 0) {
      newBlockLight = BlockLights.GetNextConfig(newBlockLight);
      nextConfigCount--;
    }

    // never go to empty lights config.
    if (!newBlockLight.back && !newBlockLight.front && !newBlockLight.left && !newBlockLight.right) {
      newBlockLight = BlockLights.GetNextConfig(newBlockLight);
    }

    List<int> keys = new List<int>(blockLights.Keys);
    BlockLights newRotatedLights = newBlockLight;
    foreach (int blockID in keys) {
      // rotate the pregenerated pattern by a random orthogonal rotation.
      int rotationCount = Random.Range(1, 4);
      while (rotationCount > 0) {
        newRotatedLights = BlockLights.GetRotatedClockwise(newRotatedLights);
        rotationCount--;
      }
      blockLights[blockID] = newRotatedLights;
    }
  }

  // patternSeen is populated in Cozmo space.
  static public bool ValidPatternSeen(out RowBlockPattern patternSeen, Robot robot, Dictionary<int, BlockLights> blockLightsLocalSpace) {
    patternSeen = new RowBlockPattern();

    // 3 to a pattern
    if (robot.markersVisibleObjects.Count < 3)
      return false;

    // check rotation alignment
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Forward;
      relativeForward.Normalize();

      if (Mathf.Abs(relativeForward.x) <= 0.94f && Mathf.Abs(relativeForward.y) <= 0.94f) {
        // non orthogonal forward vector, let's early out.
        return false;
      }
    }

    // check position alignment (within certain x threshold)
    for (int i = 0; i < robot.markersVisibleObjects.Count - 1; ++i) {
      Vector3 robotSpaceLocation0 = robot.activeBlocks[robot.markersVisibleObjects[i].ID].WorldPosition - robot.WorldPosition;
      robotSpaceLocation0 = Quaternion.Inverse(robot.Rotation) * robotSpaceLocation0;

      Vector3 robotSpaceLocation1 = robot.activeBlocks[robot.markersVisibleObjects[i + 1].ID].WorldPosition - robot.WorldPosition;
      robotSpaceLocation1 = Quaternion.Inverse(robot.Rotation) * robotSpaceLocation1;

      float block0 = Vector3.Dot(robot.activeBlocks[robot.markersVisibleObjects[i].ID].WorldPosition, robot.Forward);
      float block1 = Vector3.Dot(robot.activeBlocks[robot.markersVisibleObjects[i + 1].ID].WorldPosition, robot.Forward);

      if (Mathf.Abs(block0 - block1) > 10.0f) {
        return false;
      }
    }

    // convert get block lights in cozmo space.
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Forward;
      BlockLights blockLightCozmoSpace = GetInCozmoSpace(blockLightsLocalSpace[robot.markersVisibleObjects[i].ID], relativeForward);
      patternSeen.blocks.Add(blockLightCozmoSpace);
    }

    // make sure all of the light patterns match.
    for (int i = 0; i < patternSeen.blocks.Count - 1; ++i) {
      if (patternSeen.blocks[i].back != patternSeen.blocks[i + 1].back ||
          patternSeen.blocks[i].front != patternSeen.blocks[i + 1].front ||
          patternSeen.blocks[i].left != patternSeen.blocks[i + 1].left ||
          patternSeen.blocks[i].right != patternSeen.blocks[i + 1].right) {
        return false;
      }
    }

    // if all of the patterns match but no lights are on don't match.
    if (patternSeen.blocks[0].back == false && patternSeen.blocks[0].front == false
        && patternSeen.blocks[0].left == false && patternSeen.blocks[0].right == false) {
      return false;
    }

    return true;
  }

  static private BlockLights GetInCozmoSpace(BlockLights blockLocalSpace, Vector3 blockForward) {
    BlockLights blockLightCozmoSpace = new BlockLights();

    if (blockForward.x > 0.9f) {
      blockLightCozmoSpace.right = blockLocalSpace.front;
      blockLightCozmoSpace.back = blockLocalSpace.right;
      blockLightCozmoSpace.left = blockLocalSpace.back;
      blockLightCozmoSpace.front = blockLocalSpace.left;
    }
    else if (blockForward.x < -0.9f) {
      blockLightCozmoSpace.left = blockLocalSpace.front;
      blockLightCozmoSpace.front = blockLocalSpace.right;
      blockLightCozmoSpace.right = blockLocalSpace.back;
      blockLightCozmoSpace.back = blockLocalSpace.left;
    }
    else if (blockForward.y > 0.9f) {
      blockLightCozmoSpace.back = blockLocalSpace.front;
      blockLightCozmoSpace.left = blockLocalSpace.right;
      blockLightCozmoSpace.front = blockLocalSpace.back;
      blockLightCozmoSpace.right = blockLocalSpace.left;
    }
    else if (blockForward.y < -0.9f) {
      // same orientation so copy it over
      blockLightCozmoSpace = blockLocalSpace;
    }

    return blockLightCozmoSpace;
  }

}
