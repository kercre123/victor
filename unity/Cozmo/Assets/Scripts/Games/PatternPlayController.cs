using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  private bool animationPlaying = false;

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
  }

  private Dictionary<int, BlockLightConfig> blockLightConfigs = new Dictionary<int, BlockLightConfig>();
  private List<RowBlockPattern> seenPatterns = new List<RowBlockPattern>();

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.TappedAction += BlockTapped;
    RobotEngineManager.instance.SuccessOrFailure += RobotEngineMessages;
  }

  protected override void OnDisable() {
    base.OnDisable();
    ActiveBlock.TappedAction -= BlockTapped;
    RobotEngineManager.instance.SuccessOrFailure -= RobotEngineMessages;
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(new Color(1.0f, 1.0f, 1.0f, 1.0f));
      }
    }

    // makes sure cozmo sees all 4 blocks first.
    if (robot.activeBlocks.Count >= 4) {
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
    }
    robot.SetHeadAngle(-0.5f);
    robot.SetLiftHeight(2.0f);
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    // update lights
    foreach (KeyValuePair<int, BlockLightConfig> blockConfig in blockLightConfigs) {
      for (int i = 0; i < robot.activeBlocks[blockConfig.Key].lights.Length; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(Color.black);
      }

      // setting onColor based on if cozmo sees the block or not.
      Color onColor = Color.white;

      for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
        if (robot.markersVisibleObjects[i].ID == blockConfig.Key) {
          onColor = Color.blue;
          break;
        }
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
    if (ValidPatternSeen(out currentPattern)) {
      if (!animationPlaying) {
        if (NewPatternSeen(currentPattern)) {
          // play joy.
          SendAnimation("winMatch");
        }
        else {
          // play meh.
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

  private void BlockTapped(int blockID, int numTapped) {
    if (gameReady == false)
      return;

    // go to the next light configuration
    blockLightConfigs[blockID] = (BlockLightConfig)(((int)blockLightConfigs[blockID] + numTapped) % System.Enum.GetNames(typeof(BlockLightConfig)).Length);
  }

  private void RobotEngineMessages(bool success, RobotActionType action_type) {
    DAS.Debug("PatternPlayController", "RobotEngineMessage: " + action_type);
    if (action_type == RobotActionType.PLAY_ANIMATION) {
      animationPlaying = false;
    }
  }

  private bool ValidPatternSeen(out RowBlockPattern patternSeen) {
    patternSeen = new RowBlockPattern();

    // need at least 2 to form a pattern.
    if (robot.markersVisibleObjects.Count < 2)
      return false;

    // check rotation alignment

    // check position alignment (within certain x threshold)

    // check for pattern

    for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
      Vector3 relativeForward = robot.Rotation * robot.activeBlocks[robot.markersVisibleObjects[i].ID].Forward;
      BlockLights blockLight;

      // logic for relative LEDs... should be refactored to be not as ugly.
      switch (blockLightConfigs[robot.markersVisibleObjects[i].ID]) {
      case BlockLightConfig.NONE:
        break;
      case BlockLightConfig.ONE:
        if (relativeForward.x > 0.9f) {
          blockLight.left = true;
        }
        if (relativeForward.x < -0.9f) {
          blockLight.right = true;
        }
        if (relativeForward.y > 0.9f) {
          blockLight.front = true;
        }
        if (relativeForward.y < -0.9f) {
          blockLight.back = true;
        }
        break;
      case BlockLightConfig.TWO:
        if (relativeForward.x > 0.9f) {
          blockLight.left = true;
          blockLight.back = true;
        }
        if (relativeForward.x < -0.9f) {
          blockLight.right = true;
          blockLight.front = true;
        }
        if (relativeForward.y > 0.9f) {
          blockLight.front = true;
          blockLight.left = true;
        }
        if (relativeForward.y < -0.9f) {
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
        if (relativeForward.x < -0.9f) {
          blockLight.right = true;
          blockLight.front = true;
          blockLight.left = true;
        }
        if (relativeForward.y > 0.9f) {
          blockLight.front = true;
          blockLight.left = true;
          blockLight.back = true;
        }
        if (relativeForward.y < -0.9f) {
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
        if (relativeForward.y > 0.9f || relativeForward.y < -0.9f) {
          blockLight.front = true;
          blockLight.back = true;
        }
        break;
      }
    }

    return false;
  }

  private bool NewPatternSeen(RowBlockPattern patternSeen) {
    return seenPatterns.Contains(patternSeen);
  }

  private void SendAnimation(string animName) {
    animationPlaying = true;
    CozmoAnimation newAnimation = new CozmoAnimation();
    newAnimation.animName = animName;
    newAnimation.numLoops = 1;
    CozmoEmotionManager.instance.SendAnimation(newAnimation);
  }

}