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

  private enum InputMode {
    NONE,
    ONE,
    TILT,
    DOUBLE
  }

  private InputMode currentInputMode = InputMode.NONE;

  // TODO: Maybe refactor this into its own class?
  private Dictionary<int, BlockLights> blockLightsLocalSpace = new Dictionary<int, BlockLights>();

  // blocks in here are in cozmo space.
  private HashSet<RowBlockPattern> seenPatterns = new HashSet<RowBlockPattern>();
  private Dictionary<int, float> lastFrameZAccel = new Dictionary<int, float>();
  private Dictionary<int, float> lastTimeTapped = new Dictionary<int, float>();

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.MovedAction += BlockMoved;
    ActiveBlock.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    CozmoEmotionManager.instance.SetIdleAnimation("None");
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
      blockLightsLocalSpace.Add(activeBlock.Key, new BlockLights());
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

    SetBlockLights();

    // check cozmo vision for patterns.
    RowBlockPattern currentPattern = null;
    if (!animationPlaying && Time.time - lastAnimationFinishedTime > 2.0f) {
      if (RowBlockPattern.ValidPatternSeen(out currentPattern, robot, blockLightsLocalSpace)) {
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
          //SendAnimation("Satisfaction");
          SendAnimation("majorWinBeatBox");
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

  private void SetBlockLights() {
    // update block lights
    foreach (KeyValuePair<int, BlockLights> blockConfig in blockLightsLocalSpace) {

      // setting onColor based on if cozmo sees the block or not.
      Color enabledColor;
      Color disabledColor;

      disabledColor = new Color(0.0f, 0.0f, 0.4f, 1.0f);
      enabledColor = new Color(0.0f, 0.6f, 1.0f, 1.0f);

      for (int i = 0; i < robot.activeBlocks[blockConfig.Key].lights.Length; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(disabledColor);
      }

      for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
        if (robot.markersVisibleObjects[i].ID == blockConfig.Key) {
          if (animationPlaying) {
            enabledColor = Color.green;
          }
          else {
            enabledColor = Color.white;
          }
          break;
        }
      }
 
      if (currentInputMode == InputMode.TILT) {
        if (Time.time - lastTimeTapped[blockConfig.Key] < 0.3f || lastFrameZAccel[blockConfig.Key] < 10.0f) {
          enabledColor = new Color(1.0f, 0.5f, 0.0f, 1.0f);
          disabledColor = new Color(0.3f, 0.0f, 0.0f, 1.0f);
        }
      }

      for (int i = 0; i < 4; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(disabledColor);
        robot.activeBlocks[blockConfig.Key].lights[i].offColor = CozmoPalette.ColorToUInt(Color.black);
      }

      if (blockConfig.Value.back) {
        robot.activeBlocks[blockConfig.Key].lights[1].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.front) {
        robot.activeBlocks[blockConfig.Key].lights[3].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.left) {
        robot.activeBlocks[blockConfig.Key].lights[2].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.right) {
        robot.activeBlocks[blockConfig.Key].lights[0].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }
    }
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

    if (currentInputMode == InputMode.ONE) {
      while (tappedTimes > 0) {
        blockLightsLocalSpace[blockID] = BlockLights.GetNextConfig(blockLightsLocalSpace[blockID]);
        tappedTimes--;
      }
    }
    else if (currentInputMode == InputMode.DOUBLE) {
      if (Time.time - lastTimeTapped[blockID] < 0.4f && Time.time - lastTimeTapped[blockID] > 0.1f) {
        while (tappedTimes > 0) {
          blockLightsLocalSpace[blockID] = BlockLights.GetNextConfig(blockLightsLocalSpace[blockID]);
          tappedTimes--;
        }
      }
      lastTimeTapped[blockID] = Time.time;
    }
    else if (currentInputMode == InputMode.TILT) {
      if (Time.time - lastTimeTapped[blockID] < 0.3f || lastFrameZAccel[blockID] < 10.0f) {
        lastTimeTapped[blockID] = Time.time;
        while (tappedTimes > 0) {
          blockLightsLocalSpace[blockID] = BlockLights.GetNextConfig(blockLightsLocalSpace[blockID]);
          tappedTimes--;
        }
      }
    }
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
      foreach (KeyValuePair<int, BlockLights> block in blockLightsLocalSpace) {
        if (currentIndex == index) {
          blockLightsLocalSpace[block.Key] = BlockLights.GetNextConfig(block.Value);
          break;
        }
        currentIndex++; 
      }
    }
  }

  private void DonePlayingAnimation(bool success) {
    animationPlaying = false;
    lastAnimationFinishedTime = Time.time;
    RowBlockPattern.SetRandomConfig(robot, blockLightsLocalSpace, lastPatternSeen);
    ResetLookHeadForkLift();
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
  