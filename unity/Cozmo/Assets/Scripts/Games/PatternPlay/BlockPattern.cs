using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class BlockPattern {

  // block lights in cozmo-space.
  // front is the light facing cozmo.
  // left is the light left of cozmo etc.
  public List<BlockLights> blocks = new List<BlockLights>();
  public bool facingCozmo;
  public bool verticalStack;

  public override bool Equals(System.Object obj) {
    if (obj == null) {
      return false;
    }

    BlockPattern p = obj as BlockPattern;
    if ((System.Object)p == null) {
      return false;
    }
    return Equals(p);
  }

  public bool Equals(BlockPattern pattern) {
    if (pattern == null)
      return false;

    if (pattern.blocks.Count != blocks.Count)
      return false;

    if (pattern.facingCozmo != facingCozmo)
      return false;

    if (pattern.verticalStack != verticalStack)
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
    int x = 33;
    for (int i = 0; i < blocks.Count; ++i) {
      x ^= System.Convert.ToInt32(blocks[i].back) ^ System.Convert.ToInt32(blocks[i].front) << 1 ^
      System.Convert.ToInt32(blocks[i].left) << 2 ^ System.Convert.ToInt32(blocks[i].right) << 3 ^ i << 4;
    }
    return x;
  }

  static public void SetRandomConfig(Robot robot, Dictionary<int, BlockPatternData> blockPatternData, BlockPattern lastPatternSeen) {
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

    List<int> keys = new List<int>(blockPatternData.Keys);
    BlockLights newRotatedLights = newBlockLight;
    foreach (int blockID in keys) {
      // rotate the pregenerated pattern by a random orthogonal rotation.
      int rotationCount = Random.Range(1, 4);
      while (rotationCount > 0) {
        newRotatedLights = BlockLights.GetRotatedClockwise(newRotatedLights);
        rotationCount--;
      }
      blockPatternData[blockID].blockLightsLocalSpace = newRotatedLights;
    }
  }

  // patternSeen is populated in Cozmo space.
  static public bool ValidPatternSeen(out BlockPattern patternSeen, Robot robot, Dictionary<int, BlockPatternData> blockPatternData) {
    patternSeen = new BlockPattern();

    // 3 to a pattern
    if (robot.markersVisibleObjects.Count < 3)
      return false;

    patternSeen.facingCozmo = CheckFacingCozmo(robot);
    patternSeen.verticalStack = CheckVerticalStack(robot);

    bool rowAlignment = CheckRowAlignment(robot);

    if (patternSeen.verticalStack == false && rowAlignment == false) {
      return false;
    }

    // check rotation alignment
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Forward;
      relativeForward.Normalize();

      if (Mathf.Abs(relativeForward.x) < 0.92f && Mathf.Abs(relativeForward.y) < 0.92f && Mathf.Abs(relativeForward.z) < 0.92f) {
        // non orthogonal forward vector, let's early out. the z-axis is for if the cube is on the side and facing cozmo.
        return false;
      }
    }

    // convert get block lights in cozmo space. build out pattern seen.
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Forward;
      BlockLights blockLightCozmoSpace = GetInCozmoSpace(blockPatternData[robot.markersVisibleObjects[i].ID].blockLightsLocalSpace, relativeForward, patternSeen.facingCozmo);
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
    if (patternSeen.blocks[0].AreLightsOff()) {
      return false;
    }

    return true;
  }

  static bool CheckFacingCozmo(Robot robot) {
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeUp = Quaternion.Inverse(robot.Rotation) * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Up;
      if (relativeUp.x > -0.9f) {
        return false;
      }
    }
    return true;
  }

  static bool CheckVerticalStack(Robot robot) {

    for (int i = 0; i < robot.markersVisibleObjects.Count - 1; ++i) {
      Vector2 flatPos0 = (Vector2)(robot.activeBlocks[robot.markersVisibleObjects[i].ID].WorldPosition);
      Vector2 flatPos1 = (Vector2)(robot.activeBlocks[robot.markersVisibleObjects[i + 1].ID].WorldPosition);

      if (Vector2.Distance(flatPos0, flatPos1) > 5.0f) {
        return false;
      }
    }
    return true;
  }

  static bool CheckRowAlignment(Robot robot) {
    for (int i = 0; i < robot.markersVisibleObjects.Count - 1; ++i) {
      Vector3 obj0to1 = robot.markersVisibleObjects[i + 1].WorldPosition - robot.markersVisibleObjects[i].WorldPosition;
      obj0to1.Normalize();
      if (Vector3.Dot(obj0to1, robot.Forward) > 0.1f) {
        return false;
      }
    }
    return true;
  }

  static private BlockLights GetInCozmoSpace(BlockLights blockLocalSpace, Vector3 blockForward, bool facingCozmo) {
    BlockLights blockLightCozmoSpace = new BlockLights();

    if (facingCozmo) {
      if (blockForward.z > 0.9f) {
        blockLightCozmoSpace.right = blockLocalSpace.front;
        blockLightCozmoSpace.back = blockLocalSpace.right;
        blockLightCozmoSpace.left = blockLocalSpace.back;
        blockLightCozmoSpace.front = blockLocalSpace.left;
      }
      else if (blockForward.z < -0.9f) {
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
    }
    else {
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
    }

    return blockLightCozmoSpace;
  }

}
