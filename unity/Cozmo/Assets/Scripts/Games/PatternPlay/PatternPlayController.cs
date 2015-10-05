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
    DOUBLE,
    PHONE,
    COZMO
  }

  private InputMode currentInputMode = InputMode.PHONE;

  // blocks in here are in cozmo space.
  private HashSet<RowBlockPattern> seenPatterns = new HashSet<RowBlockPattern>();

  private Dictionary<int, BlockPatternData> blockPatternData = new Dictionary<int, BlockPatternData>();

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
      blockPatternData.Add(activeBlock.Key, new BlockPatternData(new BlockLights(), 30.0f, 0.0f));
    }
    ResetLookHeadForkLift();
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    KeyboardBlockCycle();
    if (currentInputMode == InputMode.PHONE) {
      PhoneCycle();
    }

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
      if (RowBlockPattern.ValidPatternSeen(out currentPattern, robot, blockPatternData)) {
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

    int lastTimerID = GetMostRecentMovedID();

    // update block lights
    foreach (KeyValuePair<int, BlockPatternData> blockConfig in blockPatternData) {

      // setting onColor based on if cozmo sees the block or not.
      Color enabledColor;
      Color disabledColor;

      disabledColor = new Color(0.0f, 0.0f, 0.0f, 1.0f);
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
        if (Time.time - blockConfig.Value.lastTimeTapped < 0.3f || blockConfig.Value.lastFrameZAccel < 10.0f) {
          enabledColor = new Color(1.0f, 0.5f, 0.0f, 1.0f);
          disabledColor = new Color(0.3f, 0.0f, 0.0f, 1.0f);
        }
      }

      if (currentInputMode == InputMode.PHONE) {
        if (blockConfig.Key == lastTimerID) {
          enabledColor = new Color(1.0f, 0.5f, 0.0f, 1.0f);
          disabledColor = new Color(0.3f, 0.0f, 0.0f, 1.0f);
        }
      }

      for (int i = 0; i < 4; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(disabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.back) {
        robot.activeBlocks[blockConfig.Key].lights[1].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.front) {
        robot.activeBlocks[blockConfig.Key].lights[3].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.left) {
        robot.activeBlocks[blockConfig.Key].lights[2].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.right) {
        robot.activeBlocks[blockConfig.Key].lights[0].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }
    }
  }

  private bool NextBlockConfig(ActiveBlock activeBlock) {
    if (blockPatternData[activeBlock.ID].lastFrameZAccel < -2.0f && activeBlock.zAccel > 2.0f) {
      blockPatternData[activeBlock.ID].lastFrameZAccel = activeBlock.zAccel;
      return true;
    }
    blockPatternData[activeBlock.ID].lastFrameZAccel = activeBlock.zAccel;
    return false;
  }

  private void BlockTapped(int blockID, int tappedTimes) {
    if (gameReady == false)
      return;

    if (currentInputMode == InputMode.ONE) {
      while (tappedTimes > 0) {
        blockPatternData[blockID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[blockID].blockLightsLocalSpace);
        tappedTimes--;
      }
    }
    else if (currentInputMode == InputMode.DOUBLE) {
      if (Time.time - blockPatternData[blockID].lastTimeTapped < 0.4f && Time.time - blockPatternData[blockID].lastTimeTapped > 0.1f) {
        while (tappedTimes > 0) {
          blockPatternData[blockID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[blockID].blockLightsLocalSpace);
          tappedTimes--;
        }
      }
      blockPatternData[blockID].lastTimeTapped = Time.time;
    }
    else if (currentInputMode == InputMode.TILT) {
      if (Time.time - blockPatternData[blockID].lastTimeTapped < 0.3f || blockPatternData[blockID].lastFrameZAccel < 10.0f) {
        blockPatternData[blockID].lastTimeTapped = Time.time;
        while (tappedTimes > 0) {
          blockPatternData[blockID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[blockID].blockLightsLocalSpace);
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
    blockPatternData[blockID].lastFrameZAccel = zAccel;
    blockPatternData[blockID].lastTimeTouched = Time.time;
  }

  private int GetMostRecentMovedID() {
    int lastTouchedID = -1;
    float minTime = 0.0f;
    foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData) {
      if (block.Value.lastTimeTouched > minTime && block.Value.BlockActiveTimeTouched()) {
        lastTouchedID = block.Key;
        minTime = block.Value.lastTimeTouched;
      }
    }
    return lastTouchedID;
  }

  private void PhoneCycle() {
    if (Input.GetMouseButtonDown(0)) {
      int lastTouchedID = GetMostRecentMovedID();

      if (lastTouchedID != -1) {
        blockPatternData[lastTouchedID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[lastTouchedID].blockLightsLocalSpace);
        blockPatternData[lastTouchedID].lastTimeTouched = Time.time;
      }
    }
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
      foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData) {
        if (currentIndex == index) {
          block.Value.blockLightsLocalSpace = BlockLights.GetNextConfig(block.Value.blockLightsLocalSpace);
          break;
        }
        currentIndex++; 
      }
    }
  }

  private void DonePlayingAnimation(bool success) {
    animationPlaying = false;
    lastAnimationFinishedTime = Time.time;
    if (currentInputMode == InputMode.COZMO) {
      RowBlockPattern.SetRandomConfig(robot, blockPatternData, lastPatternSeen);
    }
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
  