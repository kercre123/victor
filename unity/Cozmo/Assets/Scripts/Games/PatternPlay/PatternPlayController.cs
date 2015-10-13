using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  private int cozmoEnergyLevel = 0;
  private static int cozmoMaxEnergyLevel = 6;
  private float lastBlinkTime = 0.0f;
  private int lastMovedID = -1;
  private bool seenPattern = false;
  private bool lastSeenPatternNew = false;

  [SerializeField]
  private PatternPlayAudio patternPlayAudio;

  [SerializeField]
  private PatternPlayUIController patternPlayUIController;

  private StateMachineManager stateMachineManager = new StateMachineManager();

  private StateMachine patternPlayStateMachine = new StateMachine();

  private BlockPattern lastPatternSeen = null;

  private PatternMemory memoryBank = new PatternMemory();

  private Dictionary<int, BlockPatternData> blockPatternData = new Dictionary<int, BlockPatternData>();

  public BlockPattern GetLastPatternSeen() {
    return lastPatternSeen;
  }

  public bool SeenPattern() {
    return seenPattern;
  }

  public bool LastSeenPatternNew() {
    return lastSeenPatternNew;
  }

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.MovedAction += BlockMoving;
    ActiveBlock.StoppedAction += BlockStopped;
    ActiveBlock.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    memoryBank.Initialize();
  }

  protected override void OnDisable() {
    base.OnDisable();
    ActiveBlock.MovedAction -= BlockMoving;
    ActiveBlock.StoppedAction -= BlockStopped;
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
    patternPlayStateMachine.SetGameController(this);
    patternPlayStateMachine.SetNextState(new LookForPattern());
    stateMachineManager.AddStateMachine("PatternPlayStateMachine", patternPlayStateMachine);
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    KeyboardBlockCycle();
    PhoneCycle();

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
    BlockPattern currentPattern = null;
    seenPattern = false;

    if (BlockPattern.ValidPatternSeen(out currentPattern, robot, blockPatternData)) {
      if (!memoryBank.Contains(currentPattern)) {
        cozmoEnergyLevel++;

        if (cozmoEnergyLevel > cozmoMaxEnergyLevel) {
          cozmoEnergyLevel = cozmoMaxEnergyLevel;
        }

        DAS.Info("PatternPlayController", "New Pattern: " + "facingCozmo: " + currentPattern.facingCozmo + " vertical: " + currentPattern.verticalStack +
        " lights: " + currentPattern.blocks[0].back + " " + currentPattern.blocks[0].front + " " + currentPattern.blocks[0].left + " " + currentPattern.blocks[0].right);

        seenPattern = true;
        lastSeenPatternNew = true;
        memoryBank.Add(currentPattern);
      }
      else if (lastPatternSeen.Equals(currentPattern) == false) {
        seenPattern = true;
        lastSeenPatternNew = false;
      }

      lastPatternSeen = currentPattern;
    }

    stateMachineManager.UpdateAllMachines();

    // this may need to be moved to include other states if we want UI for them.
    patternPlayUIController.UpdateUI(memoryBank);

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

    int currentMovedID = GetMostRecentMovedID();
    if (currentMovedID != lastMovedID) {
      lastBlinkTime = Time.time;
    }

    // update block lights
    foreach (KeyValuePair<int, BlockPatternData> blockConfig in blockPatternData) {

      // setting onColor based on if cozmo sees the block or not.
      Color enabledColor;
      Color disabledColor;

      enabledColor = Color.blue;
      disabledColor = Color.black;

      for (int i = 0; i < robot.activeBlocks[blockConfig.Key].lights.Length; ++i) {
        robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(disabledColor);
      }
 
      if (blockConfig.Key == currentMovedID && blockPatternData[blockConfig.Key].moving) {
        enabledColor = Color.green;
        disabledColor = Color.green;
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

    lastMovedID = currentMovedID;
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

  }

  private void BlockMoving(int blockID, float xAccel, float yAccel, float zAccel) {
    if (gameReady == false)
      return;

    blockPatternData[blockID].moving = true;
    blockPatternData[blockID].lastFrameZAccel = zAccel;
    blockPatternData[blockID].lastTimeTouched = Time.time;
  }

  private void BlockStopped(int blockID) {
    if (gameReady == false)
      return;
    
    blockPatternData[blockID].moving = false;
  }

  private int GetMostRecentMovedID() {
    int lastTouchedID = -1;
    float minTime = 0.0f;
    foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData) {
      if (block.Value.lastTimeTouched > minTime) {
        lastTouchedID = block.Key;
        minTime = block.Value.lastTimeTouched;
      }
    }
    return lastTouchedID;
  }

  public float GetMostRecentMovedTime() {
    float minTime = 0.0f;
    foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData) {
      if (block.Value.lastTimeTouched > minTime) {
        minTime = block.Value.lastTimeTouched;
      }
    }
    return minTime;
  }

  private void PhoneCycle() {
    if (Input.GetMouseButtonDown(0)) {
      int lastTouchedID = GetMostRecentMovedID();

      if (lastTouchedID != -1) {
        
        blockPatternData[lastTouchedID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[lastTouchedID].blockLightsLocalSpace);
        blockPatternData[lastTouchedID].lastTimeTouched = Time.time;
        patternPlayAudio.PlayLightsSound(blockPatternData[lastTouchedID].blockLightsLocalSpace.NumberOfLightsOn());
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

  public void ResetLookHeadForkLift() {
    robot.SetHeadAngle(-0.1f);
    robot.SetLiftHeight(2.0f);
  }
    
}
  