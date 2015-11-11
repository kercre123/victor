using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace PatternPlay {

  public class BlockPattern {

    // block lights in cozmo-space.
    // front is the light facing cozmo.
    // left is the light left of cozmo etc.
    public List<BlockLights> Blocks = new List<BlockLights>();
    public bool VerticalStack;

    public bool FacingCozmo {
      set {
        for (int i = 0; i < Blocks.Count; i++) {
          Blocks[i].facing_cozmo = value;
        }
      }
      get {
        for (int i = 0; i < Blocks.Count; i++) {
          if (!Blocks[i].facing_cozmo) {
            return false;
          }
        }
        return true;
      }
    }

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

      if (pattern.Blocks.Count != Blocks.Count)
        return false;

      if (pattern.VerticalStack != VerticalStack)
        return false;

      for (int i = 0; i < pattern.Blocks.Count; ++i) {
        if (pattern.Blocks[i].back != Blocks[i].back ||
            pattern.Blocks[i].front != Blocks[i].front ||
            pattern.Blocks[i].left != Blocks[i].left ||
            pattern.Blocks[i].right != Blocks[i].right ||
            pattern.Blocks[i].facing_cozmo != Blocks[i].facing_cozmo) {
          return false;
        }
      }

      return true;
    }

    public override int GetHashCode() {
      int x = System.Convert.ToInt32(VerticalStack);
      for (int i = 0; i < Blocks.Count; ++i) {
        // If more attribs are added, please increase this radix value.
        x = (x * 64) + (System.Convert.ToInt32(Blocks[i].back) | System.Convert.ToInt32(Blocks[i].front) << 1 |
        System.Convert.ToInt32(Blocks[i].left) << 2 | System.Convert.ToInt32(Blocks[i].right) << 3 |
        System.Convert.ToInt32(Blocks[i].facing_cozmo) << 4);
      }
      return x;
    }

    static public void SetRandomConfig(Robot robot, Dictionary<int, BlockPatternData> blockPatternData, BlockPattern lastPatternSeen) {
      BlockLights patternLight = lastPatternSeen.Blocks[0];

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
        blockPatternData[blockID].BlockLightsLocalSpace = newRotatedLights;
      }
    }

    // patternSeen is populated in Cozmo space.
    static public bool ValidPatternSeen(out BlockPattern patternSeen, Robot robot, Dictionary<int, BlockPatternData> blockPatternData) {
      patternSeen = new BlockPattern();

      // 3 to a pattern
      if (robot.VisibleObjects.Count < 3)
        return false;

      bool facingCozmo = CheckFacingCozmo(robot);
      patternSeen.FacingCozmo = facingCozmo;
      patternSeen.VerticalStack = CheckVerticalStack(robot);

      bool rowAlignment = CheckRowAlignment(robot);

      if (patternSeen.VerticalStack == false && rowAlignment == false) {
        return false;
      }

      // check rotation alignment
      for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
        Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.LightCubes[robot.VisibleObjects[i].ID].Forward;
        relativeForward.Normalize();

        if (Mathf.Abs(relativeForward.x) < 0.92f && Mathf.Abs(relativeForward.y) < 0.92f && Mathf.Abs(relativeForward.z) < 0.92f) {
          // non orthogonal forward vector, let's early out. the z-axis is for if the cube is on the side and facing cozmo.
          return false;
        }
      }

      // convert get block lights in cozmo space. build out pattern seen.
      for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
        Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.LightCubes[robot.VisibleObjects[i].ID].Forward;
        BlockLights blockLightCozmoSpace = GetInCozmoSpace(blockPatternData[robot.VisibleObjects[i].ID].BlockLightsLocalSpace, relativeForward, facingCozmo);
        patternSeen.Blocks.Add(blockLightCozmoSpace);
      }

      // make sure all of the light patterns match.
      for (int i = 0; i < patternSeen.Blocks.Count - 1; ++i) {
        if (patternSeen.Blocks[i].back != patternSeen.Blocks[i + 1].back ||
            patternSeen.Blocks[i].front != patternSeen.Blocks[i + 1].front ||
            patternSeen.Blocks[i].left != patternSeen.Blocks[i + 1].left ||
            patternSeen.Blocks[i].right != patternSeen.Blocks[i + 1].right) {
          return false;
        }
      }

      // if all of the patterns match but no lights are on don't match.
      if (patternSeen.Blocks[0].AreLightsOff()) {
        return false;
      }

      return true;
    }

    static bool CheckFacingCozmo(Robot robot) {
      for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
        Vector3 relativeUp = Quaternion.Inverse(robot.Rotation) * robot.LightCubes[robot.VisibleObjects[i].ID].Up;
        if (relativeUp.x > -0.9f) {
          return false;
        }
      }
      return true;
    }

    static bool CheckVerticalStack(Robot robot) {

      for (int i = 0; i < robot.VisibleObjects.Count - 1; ++i) {
        Vector2 flatPos0 = (Vector2)(robot.LightCubes[robot.VisibleObjects[i].ID].WorldPosition);
        Vector2 flatPos1 = (Vector2)(robot.LightCubes[robot.VisibleObjects[i + 1].ID].WorldPosition);

        if (Vector2.Distance(flatPos0, flatPos1) > 5.0f) {
          return false;
        }
      }
      return true;
    }

    static bool CheckRowAlignment(Robot robot) {
      for (int i = 0; i < robot.VisibleObjects.Count - 1; ++i) {
        Vector3 obj0to1 = robot.VisibleObjects[i + 1].WorldPosition - robot.VisibleObjects[i].WorldPosition;
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
          blockLightCozmoSpace = blockLocalSpace.Clone() as BlockLights;
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
          // same orientation so copy it over. Make sure you deep copy!!
          blockLightCozmoSpace = blockLocalSpace.Clone() as BlockLights;
        }
      }

      return blockLightCozmoSpace;
    }

  }

}
