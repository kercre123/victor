using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  private int previousInputID = -1;
  private bool seenPattern = false;
  private bool shouldCelebrateNew = false;
  private int lastSetID = -1;
  private float lastSetTime = -100.0f;
  private float lastPatternSeen = 0.0f;
  private BlockPattern lastSeenPattern;

  // variables for autonomous pattern building
  PatternPlayAutoBuild patternPlayAutoBuild = new PatternPlayAutoBuild();

  [SerializeField]
  private PatternPlayAudio patternPlayAudio;

  [SerializeField]
  private PatternCollectionViewController patternPlayUIController;

  private StateMachineManager stateMachineManager = new StateMachineManager();

  private StateMachine patternPlayStateMachine = new StateMachine();

  private PatternMemory memoryBank = new PatternMemory();

  private Dictionary<int, BlockPatternData> blockPatternData = new Dictionary<int, BlockPatternData>();

  public void SetPatternOnBlock(int blockID, int lightCount) {

    blockPatternData[blockID].blockLightsLocalSpace.TurnOffLights();

    if (lightCount > 0) {
      blockPatternData[blockID].blockLightsLocalSpace.front = true;
      lightCount--;
    }

    if (lightCount > 0) {
      blockPatternData[blockID].blockLightsLocalSpace.left = true;
      lightCount--;
    }

    if (lightCount > 0) {
      blockPatternData[blockID].blockLightsLocalSpace.back = true;
      lightCount--;
    }

    if (lightCount > 0) {
      blockPatternData[blockID].blockLightsLocalSpace.right = true;
      lightCount--;
    }

    float blockAngleWorldSpace = robot.activeBlocks[blockID].Rotation.eulerAngles.z;
    if (blockAngleWorldSpace < 0.0f) {
      blockAngleWorldSpace += 360.0f;
    }

    if (blockAngleWorldSpace > 75.0f) {
      blockPatternData[blockID].blockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData[blockID].blockLightsLocalSpace);
    }

    if (blockAngleWorldSpace > 165.0f) {
      blockPatternData[blockID].blockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData[blockID].blockLightsLocalSpace);
    }

    if (blockAngleWorldSpace > 255.0f) {
      blockPatternData[blockID].blockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData[blockID].blockLightsLocalSpace);
    }
  }

  public Robot GetRobot() {
    return robot;
  }

  public PatternPlayAutoBuild GetAutoBuild() {
    return patternPlayAutoBuild;
  }

  public PatternMemory GetPatternMemory() {
    return memoryBank;
  }

  public void ClearBlockLights() {
    foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData) {
      kvp.Value.blockLightsLocalSpace.TurnOffLights();
    }
  }

  public bool SeenPattern() {
    return seenPattern;
  }

  public bool ShouldCelebrateNew() {
    return shouldCelebrateNew;
  }

  public void SetShouldCelebrateNew(bool shouldCelebrateNew_) {
    shouldCelebrateNew = shouldCelebrateNew_;
  }

  public float LastPatternSeenTime() {
    return lastPatternSeen;
  }

  public int SeenBlocksOverThreshold(float threshold) {
    int count = 0;
    foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData) {
      if (kvp.Value.seenAccumulator > threshold && kvp.Value.blockLightsLocalSpace.AreLightsOff() == false) {
        count++;
      }
    }
    return count;
  }

  public bool HasVerticalBlock() {
    for (int i = 0; i < robot.visibleObjects.Count; ++i) {
      Vector3 rel = robot.visibleObjects[i].WorldPosition - robot.WorldPosition;
      if (rel.z > 35.0f) {
        return true;
      }
    }
    return false;
  }

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.MovedAction += BlockMoving;
    ActiveBlock.StoppedAction += BlockStopped;
    ActiveBlock.TappedAction += BlockTapped;
    robot.StopFaceAwareness();
    memoryBank.Initialize();
    patternPlayAutoBuild.controller = this;
    robot.SetBehaviorSystem(true);
    robot.ActivateBehaviorChooser(BehaviorChooserType.Selection);
    robot.ExecuteBehavior(BehaviorType.NoneBehavior);
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

    patternPlayUIController.Initialize(memoryBank);

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      blockPatternData.Add(activeBlock.Key, new BlockPatternData(new BlockLights(), 30.0f, 0.0f));
    }
    ResetLookHeadForkLift();

    // setup pattern play state machine
    patternPlayStateMachine.SetGameController(this);
    patternPlayStateMachine.SetNextState(new LookForPattern());
    stateMachineManager.AddStateMachine("PatternPlayStateMachine", patternPlayStateMachine);

    // set idle parameters
    Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = {
      Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementDurationMax_ms,
      Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementStraightFraction,
      Anki.Cozmo.LiveIdleAnimationParameter.HeadAngleVariability_deg,
      Anki.Cozmo.LiveIdleAnimationParameter.LiftHeightVariability_mm
    };

    float[] paramValues = {
      3.0f,
      0.2f,
      5.0f,
      0.0f
    };

    CozmoEmotionManager.instance.SetLiveIdleAnimationParameters(paramNames, paramValues);

  }

  private void DebugObjectTracking() {
    Debug.Log("visible: " + robot.visibleObjects.Count);
    Debug.Log("seen:    " + robot.seenObjects.Count);
    Debug.Log("dirty:   " + robot.dirtyObjects.Count);
    Debug.Log("------------------------------");
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    if (Input.GetKey(KeyCode.Space)) {
      DebugObjectTracking();
    }
   
    // inputs for setting block lights
    KeyboardBlockCycle();
    PhoneCycle();

    // actually set the lights on the physical blocks.
    SetBlockLights();

    // detect patterns based on cozmo's vision
    DetectPatterns();

    SeenAccumulatorCompute();

    // update cozmo's behavioral state machine for pattern play.
    stateMachineManager.UpdateAllMachines();

  }

  private void SeenAccumulatorCompute() {
    foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData) {
      bool isVisible = false;
      for (int i = 0; i < robot.visibleObjects.Count; ++i) {
        if (robot.visibleObjects[i].ID == kvp.Key) {
          isVisible = true;
          break;
        }
      }
      if (isVisible) {
        kvp.Value.seenAccumulator = Mathf.Min(1.5f, kvp.Value.seenAccumulator + Time.deltaTime);
      }
      else {
        kvp.Value.seenAccumulator = Mathf.Max(0.0f, kvp.Value.seenAccumulator - Time.deltaTime * 2.0f);
      }

    }

  }

  public BlockPatternData GetBlockPatternData(int blockID) {
    return blockPatternData[blockID];
  }

  private void DetectPatterns() {
    // check cozmo vision for patterns.
    BlockPattern currentPattern = null;
    seenPattern = false;

    if (BlockPattern.ValidPatternSeen(out currentPattern, robot, blockPatternData)) {
      seenPattern = true;
      lastPatternSeen = Time.time;
      if (memoryBank.AddSeen(currentPattern)) {
        shouldCelebrateNew = true;
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

  // sets the lights on the physical blocks based on configurations in Pattern Play.
  private void SetBlockLights() {

    int currentInputID = SelectNewInputCandidate();

    if (currentInputID != previousInputID) {
      lastSetTime = -100.0f;
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

      if (blockConfig.Key == currentInputID && blockPatternData[blockConfig.Key].moving && Time.time - lastSetTime > 5.0f) {
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

      // if cozmo is building his own pattern, then set the "seen" non-dirty blocks that are not
      // in a pattern yet to white.
      bool autoBuilding = patternPlayAutoBuild.autoBuilding;
      bool nonDirtySeen = robot.seenObjects.Contains(robot.activeBlocks[blockConfig.Key]);
      bool notInNeatList = patternPlayAutoBuild.NeatListContains(robot.activeBlocks[blockConfig.Key]) == false;
      bool notCarrying = true;
      if (patternPlayAutoBuild.GetHeldObject() != null) {
        if (blockConfig.Key != patternPlayAutoBuild.GetHeldObject().ID) {
          notCarrying = false;
        }
      }

      if (autoBuilding && nonDirtySeen && notInNeatList && notCarrying) {
        for (int i = 0; i < robot.activeBlocks[blockConfig.Key].lights.Length; ++i) {
          robot.activeBlocks[blockConfig.Key].lights[i].onColor = CozmoPalette.ColorToUInt(Color.white);
        }
      }

    }

    previousInputID = currentInputID;
  }

  private void BlockTapped(int blockID, int tappedTimes) {

  }

  private void BlockMoving(int blockID, float xAccel, float yAccel, float zAccel) {
    if (gameReady == false)
      return;

    blockPatternData[blockID].moving = true;
    blockPatternData[blockID].lastFrameZAccel = zAccel;
    blockPatternData[blockID].lastTimeTouched = Time.time;
    patternPlayAutoBuild.ObjectMoved(blockID);
  }

  private void BlockStopped(int blockID) {
    if (gameReady == false)
      return;

    blockPatternData[blockID].moving = false;
  }

  private int SelectNewInputCandidate() {
    int lastTouchedID = -1;
    float minTime = -float.MaxValue;
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
      int lastTouchedID = SelectNewInputCandidate();

      if (lastTouchedID != -1) {
        
        blockPatternData[lastTouchedID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[lastTouchedID].blockLightsLocalSpace);
        blockPatternData[lastTouchedID].lastTimeTouched = Time.time;
        patternPlayAudio.PlayLightsSound(blockPatternData[lastTouchedID].blockLightsLocalSpace.NumberOfLightsOn());
        LightSet(lastTouchedID);
      }
    }
  }

  private void LightSet(int blockID) {
    lastSetID = blockID;
    lastSetTime = Time.time;
  }

  private void KeyboardBlockCycle() {
    int blockIndex = -1;

    // figure out which block to set
    if (Input.GetKeyDown(KeyCode.Alpha1)) {
      blockIndex = 0;
    }

    if (Input.GetKeyDown(KeyCode.Alpha2)) {
      blockIndex = 1;
    }

    if (Input.GetKeyDown(KeyCode.Alpha3)) {
      blockIndex = 2;
    }

    if (Input.GetKeyDown(KeyCode.Alpha4)) {
      blockIndex = 3;
    }

    if (blockIndex != -1) {
      blockPatternData[blockIndex].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData[blockIndex].blockLightsLocalSpace);
      LightSet(blockIndex);
    }
  }

  public void ResetLookHeadForkLift() {
    robot.SetHeadAngle(-0.1f);
    robot.SetLiftHeight(0.0f);
  }

}
  