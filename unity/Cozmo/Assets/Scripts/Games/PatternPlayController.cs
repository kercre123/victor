using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

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
      if (NewPatternSeen()) {
        // play joy.
      }
      else {
        // play meh.
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

  }

  private bool ValidPatternSeen(out RowBlockPattern patternSeen) {
    patternSeen = new RowBlockPattern();

    // check rotation alignment (within certain angle mod by 90 degrees)

    // check position alignment (within certain x threshold)

    // check for pattern


    return false;
  }

  private bool NewPatternSeen() {
    return false;
  }

}