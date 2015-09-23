using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  private bool animationPlaying = false;
  private float lastAnimationFinishedTime = 0.0f;
  private int cozmoEnergyLevel = 0;
  private static int cozmoMaxEnergyLevel = 6;

  private RowBlockPattern lastPatternSeen = null;

  // block lights relative to cozmo.
  // front is the light facing cozmo.
  // left is the light left of cozmo etc.
  private struct BlockLights {
    public bool front;
    public bool back;
    public bool left;
    public bool right;
  }

  // light configuration enums.
  private enum BlockLightConfig {
    NONE,
    ONE,
    TWO,
    THREE,
    FOUR,
    OPPOSITE
  }

  private class RowBlockPattern {
    // left to right pattern relative to cozmo.
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

  }

  private Dictionary<int, BlockLightConfig> blockLightConfigs = new Dictionary<int, BlockLightConfig>();
  private HashSet<RowBlockPattern> seenPatterns = new HashSet<RowBlockPattern>();
  private Dictionary<int, float> lastFrameZAccel = new Dictionary<int, float>();
  private Dictionary<int, float> lastTimeTapped = new Dictionary<int, float>();

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.MovedAction += BlockMoved;
    ActiveBlock.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
  }

  protected override void OnDisable() {
    base.OnDisable();
    ActiveBlock.MovedAction -= BlockMoved;
    ActiveBlock.TappedAction -= BlockTapped;
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.blue);
      }
    }

    // makes sure cozmo sees all 4 blocks first.
    if (robot.activeBlocks.Count >= 3) {
      gameReady = true;
    }
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
  }

  protected override void Enter_PRE_GAME() {
    base.Enter_PRE_GAME();
  }

  protected override void Update_PRE_GAME() {
    base.Update_PRE_GAME();
  }

  protected override void Exit_PRE_GAME() {
    base.Exit_PRE_GAME();
  }

  protected override void Enter_PLAYING() {
    base.Enter_PLAYING();

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      blockLightConfigs.Add(activeBlock.Key, BlockLightConfig.NONE);
      lastFrameZAccel.Add(activeBlock.Key, 30.0f);
      lastTimeTapped.Add(activeBlock.Key, 0.0f);
    }
    ResetLookHeadForkLift();
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    KeyboardBlockCycle();

    Color[] levelColors = { new Color(0.8f, 0.5f, 0.0f, 1.0f), Color.green };

    // update backpack lights
    // iterate through each of the lights and determine if
    // the light should be on, and if so, which color.

    for (int i = 0; i < 3; ++i) {
      int localEnergyLevel = cozmoEnergyLevel;
      int localLoopCount = 0;
      robot.lights[i].onColor = CozmoPalette.ColorToUInt(Color.black);
      do {
        if (localEnergyLevel > i) {
          robot.lights[i].onColor = CozmoPalette.ColorToUInt(levelColors[localLoopCount]);
        }
        localLoopCount++;
        localEnergyLevel -= 3;
      } while (localEnergyLevel > i && localLoopCount < levelColors.Length);
    }

    // update block lights
    foreach (KeyValuePair<int, BlockLightConfig> blockConfig in blockLightConfigs) {
      for (int i = 0; i < robot.activeBlocks[blockConfig.Key].lights.Length; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(new Color(0.0f, 0.0f, 0.2f, 1.0f));
      }

      // setting onColor based on if cozmo sees the block or not.
      Color onColor = Color.blue;
      Color offColor = new Color(0.0f, 0.0f, 0.0f, 1.0f);

      for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
        if (robot.markersVisibleObjects[i].ID == blockConfig.Key) {
          if (animationPlaying) {
            onColor = Color.green;
          }
          else {
            onColor = Color.white;
            offColor = Color.black;
          }
          break;
        }
      }

      /*if (Time.time - lastTimeTapped[blockConfig.Key] < 0.5f || lastFrameZAccel[blockConfig.Key] < 10.0f) {
        offColor = new Color(0.2f, 0.0f, 0.0f, 1.0f);
        onColor = new Color(1.0f, 0.5f, 0.0f, 1.0f);
      }*/

      for (int i = 0; i < 4; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(offColor);
      }

      switch (blockConfig.Value) {
      case BlockLightConfig.NONE:
        break;
      case BlockLightConfig.ONE:
        robot.activeBlocks[blockConfig.Key].lights[1].onColor = CozmoPalette.ColorToUInt(onColor);
        break;
      case BlockLightConfig.TWO:
        robot.activeBlocks[blockConfig.Key].lights[0].onColor = CozmoPalette.ColorToUInt(onColor);
        robot.activeBlocks[blockConfig.Key].lights[1].onColor = CozmoPalette.ColorToUInt(onColor);
        break;
      case BlockLightConfig.THREE:
        robot.activeBlocks[blockConfig.Key].lights[0].onColor = CozmoPalette.ColorToUInt(onColor);
        robot.activeBlocks[blockConfig.Key].lights[1].onColor = CozmoPalette.ColorToUInt(onColor);
        robot.activeBlocks[blockConfig.Key].lights[3].onColor = CozmoPalette.ColorToUInt(onColor);
        break;
      case BlockLightConfig.FOUR:
        for (int i = 0; i < robot.activeBlocks[blockConfig.Key].lights.Length; ++i) {
          robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(onColor);
        }
        break;
      case BlockLightConfig.OPPOSITE:
        robot.activeBlocks[blockConfig.Key].lights[1].onColor = CozmoPalette.ColorToUInt(onColor);
        robot.activeBlocks[blockConfig.Key].lights[3].onColor = CozmoPalette.ColorToUInt(onColor);
        break;
      }
    }

    // check cozmo vision for patterns.
    RowBlockPattern currentPattern = null;
    if (!animationPlaying && Time.time - lastAnimationFinishedTime > 2.0f) {
      if (ValidPatternSeen(out currentPattern)) {
        lastPatternSeen = currentPattern;
        if (!PatternSeen(currentPattern)) {
          cozmoEnergyLevel++;

          if (cozmoEnergyLevel > cozmoMaxEnergyLevel) {
            cozmoEnergyLevel = cozmoMaxEnergyLevel;
          }

          if (cozmoEnergyLevel % 3 == 0) {
            SendAnimation("Celebration");
          }
          else {
            SendAnimation("majorWinBeatBox");
          }
          seenPatterns.Add(currentPattern);
        }
        else if (lastPatternSeen.Equals(currentPattern) == false) {
          SendAnimation("Satisfaction");
        }
      }
    }

  }

  protected override void Exit_PLAYING(bool overrideStars = false) {
    base.Exit_PLAYING();
  }

  protected override void Enter_RESULTS() {
    base.Enter_RESULTS();
  }

  protected override void Update_RESULTS() {
    base.Update_RESULTS();
  }

  protected override void Exit_RESULTS() {
    base.Exit_RESULTS();
  }

  protected override bool IsPreGameCompleted() {
    return true;
  }

  protected override bool IsGameReady() {
    return gameReady;
  }

  protected override bool IsGameOver() {
    return gameOver;
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();
  }

  private bool NextBlockConfig(ActiveBlock activeBlock) {
    if (lastFrameZAccel[activeBlock.ID] < -2.0f && activeBlock.zAccel > 2.0f) {
      lastFrameZAccel[activeBlock.ID] = activeBlock.zAccel;
      return true;
    }
    lastFrameZAccel[activeBlock.ID] = activeBlock.zAccel;
    return false;
  }

  private void BlockTapped(int blockID, int tappedTimes) {
    if (gameReady == false)
      return;

    //blockLightConfigs[blockID] = (BlockLightConfig)(((int)blockLightConfigs[blockID] + tappedTimes) % System.Enum.GetNames(typeof(BlockLightConfig)).Length);

    if (Time.time - lastTimeTapped[blockID] < 0.4f && Time.time - lastTimeTapped[blockID] > 0.15f) {
      blockLightConfigs[blockID] = (BlockLightConfig)(((int)blockLightConfigs[blockID] + 1) % System.Enum.GetNames(typeof(BlockLightConfig)).Length);
    }
    lastTimeTapped[blockID] = Time.time;

    /*if (Time.time - lastTimeTapped[blockID] < 0.5f || lastFrameZAccel[blockID] < 10.0f) {
      lastTimeTapped[blockID] = Time.time;
      blockLightConfigs[blockID] = (BlockLightConfig)(((int)blockLightConfigs[blockID] + tappedTimes) % System.Enum.GetNames(typeof(BlockLightConfig)).Length);
    }*/
  }

  private void BlockMoved(int blockID, float xAccel, float yAccel, float zAccel) {
    if (gameReady == false)
      return;

    if (animationPlaying) {
      // TODO: Interrupt animation because cozmo is upset the pattern may have been messed up.
    }

    Debug.Log(blockID + " : " + xAccel + " " + yAccel + " " + zAccel);
    lastFrameZAccel[blockID] = zAccel;
  }

  private void KeyboardBlockCycle() {
    int index = -1;
    int currentIndex = 0;
    if (Input.GetKeyDown(KeyCode.Alpha1)) {
      index = 0;
    }

    if (Input.GetKeyDown(KeyCode.Alpha2)) {
      index = 1;
    }

    if (Input.GetKeyDown(KeyCode.Alpha3)) {
      index = 2;
    }

    if (Input.GetKeyDown(KeyCode.Alpha4)) {
      index = 3;
    }

    if (index != -1) {
      foreach (KeyValuePair<int, BlockLightConfig> block in blockLightConfigs) {
        if (currentIndex == index) {
          blockLightConfigs[block.Key] = (BlockLightConfig)(((int)block.Value + 1) % System.Enum.GetNames(typeof(BlockLightConfig)).Length);
          break;
        }
        currentIndex++; 
      }

    }
  }

  private void DonePlayingAnimation(bool success) {
    animationPlaying = false;
    lastAnimationFinishedTime = Time.time;
    ResetLookHeadForkLift();
  }

  private bool ValidPatternSeen(out RowBlockPattern patternSeen) {
    patternSeen = new RowBlockPattern();

    // need at least 2 to form a pattern.
    if (robot.markersVisibleObjects.Count < 2)
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

    // build relative light pattern array
    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeForward = Quaternion.Inverse(robot.Rotation) * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Forward;
      BlockLights blockLight = new BlockLights();

      // logic for relative LEDs... should be refactored to be not as ugly.
      switch (blockLightConfigs[robot.markersVisibleObjects[i].ID]) {
      case BlockLightConfig.NONE:
        break;
      case BlockLightConfig.ONE:
        if (relativeForward.x > 0.9f) {
          blockLight.left = true;
        }
        else if (relativeForward.x < -0.9f) {
          blockLight.right = true;
        }
        else if (relativeForward.y > 0.9f) {
          blockLight.front = true;
        }
        else if (relativeForward.y < -0.9f) {
          blockLight.back = true;
        }
        break;
      case BlockLightConfig.TWO:
        if (relativeForward.x > 0.9f) {
          blockLight.left = true;
          blockLight.back = true;
        }
        else if (relativeForward.x < -0.9f) {
          blockLight.right = true;
          blockLight.front = true;
        }
        else if (relativeForward.y > 0.9f) {
          blockLight.front = true;
          blockLight.left = true;
        }
        else if (relativeForward.y < -0.9f) {
          blockLight.back = true;
          blockLight.right = true;
        }
        break;
      case BlockLightConfig.THREE:
        if (relativeForward.x > 0.9f) {
          blockLight.left = true;
          blockLight.back = true;
          blockLight.right = true;
        }
        else if (relativeForward.x < -0.9f) {
          blockLight.right = true;
          blockLight.front = true;
          blockLight.left = true;
        }
        else if (relativeForward.y > 0.9f) {
          blockLight.front = true;
          blockLight.left = true;
          blockLight.back = true;
        }
        else if (relativeForward.y < -0.9f) {
          blockLight.back = true;
          blockLight.right = true;
          blockLight.front = true;
        }
        break;
      case BlockLightConfig.FOUR:
        blockLight.back = true;
        blockLight.front = true;
        blockLight.left = true;
        blockLight.right = true;
        break;
      case BlockLightConfig.OPPOSITE:
        if (relativeForward.x > 0.9f || relativeForward.x < -0.9f) {
          blockLight.left = true;
          blockLight.right = true;
        }
        else if (relativeForward.y > 0.9f || relativeForward.y < -0.9f) {
          blockLight.front = true;
          blockLight.back = true;
        }
        break;
      }
      patternSeen.blocks.Add(blockLight);
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

  private bool PatternSeen(RowBlockPattern patternSeen) {
    foreach (RowBlockPattern pattern in seenPatterns) {
      if (pattern.Equals(patternSeen)) {
        return true;
      }
    }
    return false;
  }

  private void SendAnimation(string animName) {
    animationPlaying = true;
    robot.SendAnimation(animName, DonePlayingAnimation);
  }

  private void ResetLookHeadForkLift() {
    robot.SetHeadAngle(-0.5f);
    robot.SetLiftHeight(2.0f);
  }
    
}
  