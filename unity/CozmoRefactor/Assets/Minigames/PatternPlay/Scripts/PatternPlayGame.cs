using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayGame : GameBase {

  StateMachineManager patternPlayStateMachineManager_ = new StateMachineManager();
  StateMachine patternPlayStateMachine_ = new StateMachine();

  private Dictionary<int, BlockPatternData> blockPatternData_ = new Dictionary<int, BlockPatternData>();
  private PatternMemory memoryBank_ = new PatternMemory();
  private bool initialCubesDone_ = false;
  private float lastSetTime_ = -100.0f;
  private int previousInputID_ = -1;
  private bool seenPattern_ = false;
  private float lastPatternSeen_ = 0.0f;
  private bool shouldCelebrateNew_ = false;

  // variables for autonomous pattern building
  private PatternPlayAutoBuild patternPlayAutoBuild_ = new PatternPlayAutoBuild();

  private PatternPlayAudio patternPlayAudio_;

  void Start() {
    DAS.Info("PatternPlayGame", "Game Created");
    patternPlayStateMachine_.SetGameRef(this);
    patternPlayStateMachineManager_.AddStateMachine("PatternPlayStateMachine", patternPlayStateMachine_);
    InitialCubesState initCubeState = new InitialCubesState();
    initCubeState.InitialCubeRequirements(new LookForPatternState(), 3, InitialCubesDone);
    patternPlayStateMachine_.SetNextState(initCubeState);

    robot.SetBehaviorSystem(true);
    robot.ActivateBehaviorChooser(BehaviorChooserType.Selection);
    robot.ExecuteBehavior(BehaviorType.NoneBehavior);
    robot.StopFaceAwareness();

    memoryBank_.Initialize();

    patternPlayAudio_ = GetComponent<PatternPlayAudio>();
  }

  void Update() {
    
    // update cozmo's behavioral state machine for pattern play.
    // this drives the initialcubestate machine so has to be before
    // the return check.
    patternPlayStateMachineManager_.UpdateAllMachines();

    if (!initialCubesDone_) {
      return;
    }

    // Check Keyboard for Input to set block lights.
    // useful for testing using webots.
    KeyboardBlockCycle();
    PhoneCycle();

    // actually set the lights on the physical blocks.
    SetBlockLights();

    // detect patterns based on cozmo's vision
    DetectPatterns();

    // compute last seen accumulators. this is used
    // for cozmo responding to new blocks seen that are
    // in pattern.
    SeenAccumulatorCompute();

  }

  public void SetShouldCelebrateNew(bool shouldCelebrateNew) {
    shouldCelebrateNew_ = shouldCelebrateNew;
  }

  public bool ShouldCelebrateNew() {
    return shouldCelebrateNew_;
  }

  public void ClearBlockLights() {
    foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData_) {
      kvp.Value.blockLightsLocalSpace.TurnOffLights();
    }
  }

  public void ResetLookHeadForkLift() {
    robot.SetHeadAngle(-0.1f);
    robot.SetLiftHeight(0.0f);
  }

  public void InitialCubesDone() {
    initialCubesDone_ = true;

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.ActiveBlocks) {
      blockPatternData_.Add(activeBlock.Key, new BlockPatternData(new BlockLights(), 30.0f, 0.0f));
    }
    ResetLookHeadForkLift();


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

    robot.SetLiveIdleAnimationParameters(paramNames, paramValues);
  }

  public float LastPatternSeenTime() {
    return lastPatternSeen_;
  }

  public float GetMostRecentMovedTime() {
    float minTime = 0.0f;
    foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData_) {
      if (block.Value.lastTimeTouched > minTime) {
        minTime = block.Value.lastTimeTouched;
      }
    }
    return minTime;
  }

  public BlockPatternData GetBlockPatternData(int blockID) {
    return blockPatternData_[blockID];
  }

  public int SeenBlocksOverThreshold(float threshold) {
    int count = 0;
    foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData_) {
      if (kvp.Value.seenAccumulator > threshold && kvp.Value.blockLightsLocalSpace.AreLightsOff() == false) {
        count++;
      }
    }
    return count;
  }

  public bool SeenPattern() {
    return seenPattern_;
  }

  public bool HasVerticalStack() {
    for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
      Vector3 rel = robot.VisibleObjects[i].WorldPosition - robot.WorldPosition;
      if (rel.z > 35.0f) {
        return true;
      }
    }
    return false;
  }

  public PatternMemory GetPatternMemory() {
    return memoryBank_;
  }

  public PatternPlayAutoBuild GetAutoBuild() {
    return patternPlayAutoBuild_;
  }

  public void SetPatternOnBlock(int blockID, int lightCount) {

    blockPatternData_[blockID].blockLightsLocalSpace.TurnOffLights();

    if (lightCount > 0) {
      blockPatternData_[blockID].blockLightsLocalSpace.front = true;
      lightCount--;
    }

    if (lightCount > 0) {
      blockPatternData_[blockID].blockLightsLocalSpace.left = true;
      lightCount--;
    }

    if (lightCount > 0) {
      blockPatternData_[blockID].blockLightsLocalSpace.back = true;
      lightCount--;
    }

    if (lightCount > 0) {
      blockPatternData_[blockID].blockLightsLocalSpace.right = true;
      lightCount--;
    }

    float blockAngleWorldSpace = robot.ActiveBlocks[blockID].Rotation.eulerAngles.z;
    if (blockAngleWorldSpace < 0.0f) {
      blockAngleWorldSpace += 360.0f;
    }

    if (blockAngleWorldSpace > 75.0f) {
      blockPatternData_[blockID].blockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData_[blockID].blockLightsLocalSpace);
    }

    if (blockAngleWorldSpace > 165.0f) {
      blockPatternData_[blockID].blockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData_[blockID].blockLightsLocalSpace);
    }

    if (blockAngleWorldSpace > 255.0f) {
      blockPatternData_[blockID].blockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData_[blockID].blockLightsLocalSpace);
    }
  }

  private void SeenAccumulatorCompute() {
    foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData_) {
      bool isVisible = false;
      for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
        if (robot.VisibleObjects[i].ID == kvp.Key) {
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

  private void PhoneCycle() {
    if (Input.GetMouseButtonDown(0)) {
      int lastTouchedID = SelectNewInputCandidate();

      if (lastTouchedID != -1) {
        blockPatternData_[lastTouchedID].blockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData_[lastTouchedID].blockLightsLocalSpace);
        blockPatternData_[lastTouchedID].lastTimeTouched = Time.time;
        patternPlayAudio_.PlayLightsSound(blockPatternData_[lastTouchedID].blockLightsLocalSpace.NumberOfLightsOn());
        LightSet(lastTouchedID);
      }
    }
  }

  // sets the lights on the physical blocks based on configurations in Pattern Play.
  private void SetBlockLights() {

    int currentInputID = SelectNewInputCandidate();

    if (currentInputID != previousInputID_) {
      lastSetTime_ = -100.0f;
    }

    // update block lights
    foreach (KeyValuePair<int, BlockPatternData> blockConfig in blockPatternData_) {

      // setting onColor based on if cozmo sees the block or not.
      Color enabledColor;
      Color disabledColor;

      enabledColor = Color.blue;
      disabledColor = Color.black;

      for (int i = 0; i < robot.ActiveBlocks[blockConfig.Key].Lights.Length; ++i) {
        robot.ActiveBlocks[blockConfig.Key].Lights[i].onColor = CozmoPalette.ColorToUInt(disabledColor);
      }

      if (blockConfig.Key == currentInputID && blockPatternData_[blockConfig.Key].moving && Time.time - lastSetTime_ > 5.0f) {
        enabledColor = Color.green;
        disabledColor = Color.green;
      }

      for (int i = 0; i < 4; ++i) {
        robot.ActiveBlocks[blockConfig.Key].Lights[i].onColor = CozmoPalette.ColorToUInt(disabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.back) {
        robot.ActiveBlocks[blockConfig.Key].Lights[1].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.front) {
        robot.ActiveBlocks[blockConfig.Key].Lights[3].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.left) {
        robot.ActiveBlocks[blockConfig.Key].Lights[2].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      if (blockConfig.Value.blockLightsLocalSpace.right) {
        robot.ActiveBlocks[blockConfig.Key].Lights[0].onColor = CozmoPalette.ColorToUInt(enabledColor);
      }

      // if cozmo is building his own pattern, then set the "seen" non-dirty blocks that are not
      // in a pattern yet to white.
      bool autoBuilding = patternPlayAutoBuild_.autoBuilding;
      bool nonDirtySeen = robot.SeenObjects.Contains(robot.ActiveBlocks[blockConfig.Key]);
      bool notInNeatList = patternPlayAutoBuild_.NeatListContains(robot.ActiveBlocks[blockConfig.Key]) == false;
      bool notCarrying = true;
      if (patternPlayAutoBuild_.GetHeldObject() != null) {
        if (blockConfig.Key != patternPlayAutoBuild_.GetHeldObject().ID) {
          notCarrying = false;
        }
      }

      if (autoBuilding && nonDirtySeen && notInNeatList && notCarrying) {
        for (int i = 0; i < robot.ActiveBlocks[blockConfig.Key].Lights.Length; ++i) {
          robot.ActiveBlocks[blockConfig.Key].Lights[i].onColor = CozmoPalette.ColorToUInt(Color.white);
        }
      }

    }

    previousInputID_ = currentInputID;
  }

  private void DetectPatterns() {
    // check cozmo vision for patterns.
    BlockPattern currentPattern = null;
    seenPattern_ = false;

    if (BlockPattern.ValidPatternSeen(out currentPattern, robot, blockPatternData_)) {
      seenPattern_ = true;
      lastPatternSeen_ = Time.time;
      if (memoryBank_.AddSeen(currentPattern)) {
        shouldCelebrateNew_ = true;
      }
    }
  }

  private int SelectNewInputCandidate() {
    int lastTouchedID = -1;
    float minTime = -float.MaxValue;
    foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData_) {
      if (block.Value.lastTimeTouched > minTime) {
        lastTouchedID = block.Key;
        minTime = block.Value.lastTimeTouched;
      }
    }
    return lastTouchedID;
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
      int i = 0;
      foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData_) {
        if (i == blockIndex) {
          kvp.Value.blockLightsLocalSpace = BlockLights.GetNextConfig(kvp.Value.blockLightsLocalSpace);
          break;
        }
        ++i;
      }
      LightSet(blockIndex);
    }
  }

  private void LightSet(int blockID) {
    lastSetTime_ = Time.time;
  }
}
