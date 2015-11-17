using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace PatternPlay {

  public class PatternPlayGame : GameBase {

    [SerializeField]
    private PatternCollectionViewController viewControllerPrefab_;
    private PatternCollectionViewController viewControllerInstance_;

    private StateMachineManager patternPlayStateMachineManager_ = new StateMachineManager();
    private StateMachine patternPlayStateMachine_ = new StateMachine();

    private Dictionary<int, BlockPatternData> blockPatternData_ = new Dictionary<int, BlockPatternData>();
    private PatternMemory patternMemory_ = new PatternMemory();
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

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(BehaviorType.NoneBehavior);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      patternMemory_.Initialize();

      GameObject viewControllerObject = Instantiate(viewControllerPrefab_.gameObject);
      viewControllerObject.transform.SetParent(transform, false);
      viewControllerInstance_ = viewControllerObject.GetComponent<PatternCollectionViewController>();
      viewControllerInstance_.Initialize(patternMemory_);

      patternPlayAudio_ = GetComponent<PatternPlayAudio>();

      CreateDefaultQuitButton();
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

    public override void CleanUp() {
      if (viewControllerInstance_ != null) {
        Destroy(viewControllerInstance_.gameObject);
      }
      DestroyDefaultQuitButton();
    }

    public void SetShouldCelebrateNew(bool shouldCelebrateNew) {
      shouldCelebrateNew_ = shouldCelebrateNew;
    }

    public bool ShouldCelebrateNew() {
      return shouldCelebrateNew_;
    }

    public void ClearBlockLights() {
      foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData_) {
        kvp.Value.BlockLightsLocalSpace.TurnOffLights();
      }
    }

    public void ResetLookHeadForkLift() {
      CurrentRobot.SetHeadAngle(-0.1f);
      CurrentRobot.SetLiftHeight(0.0f);
    }

    public void InitialCubesDone() {
      initialCubesDone_ = true;

      LightCube.OnMovedAction += HandleBlockMoving;
      LightCube.OnStoppedAction += HandleBlockStopped;

      foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
        blockPatternData_.Add(lightCube.Key, new BlockPatternData(new BlockLights(), 30.0f, 0.0f));
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

      CurrentRobot.SetLiveIdleAnimationParameters(paramNames, paramValues);
    }

    public float LastPatternSeenTime() {
      return lastPatternSeen_;
    }

    public float GetMostRecentMovedTime() {
      float minTime = 0.0f;
      foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData_) {
        if (block.Value.LastTimeTouched > minTime) {
          minTime = block.Value.LastTimeTouched;
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
        if (kvp.Value.SeenAccumulator > threshold && kvp.Value.BlockLightsLocalSpace.AreLightsOff() == false) {
          count++;
        }
      }
      return count;
    }

    public bool SeenPattern() {
      return seenPattern_;
    }

    public bool HasVerticalStack() {
      for (int i = 0; i < CurrentRobot.VisibleObjects.Count; ++i) {
        Vector3 rel = CurrentRobot.VisibleObjects[i].WorldPosition - CurrentRobot.WorldPosition;
        if (rel.z > 35.0f) {
          return true;
        }
      }
      return false;
    }

    public PatternMemory GetPatternMemory() {
      return patternMemory_;
    }

    public PatternPlayAutoBuild GetAutoBuild() {
      return patternPlayAutoBuild_;
    }

    public void SetPatternOnBlock(int blockID, int lightCount) {

      blockPatternData_[blockID].BlockLightsLocalSpace.TurnOffLights();

      if (lightCount > 0) {
        blockPatternData_[blockID].BlockLightsLocalSpace.front = true;
        lightCount--;
      }

      if (lightCount > 0) {
        blockPatternData_[blockID].BlockLightsLocalSpace.left = true;
        lightCount--;
      }

      if (lightCount > 0) {
        blockPatternData_[blockID].BlockLightsLocalSpace.back = true;
        lightCount--;
      }

      if (lightCount > 0) {
        blockPatternData_[blockID].BlockLightsLocalSpace.right = true;
        lightCount--;
      }

      float blockAngleWorldSpace = CurrentRobot.LightCubes[blockID].Rotation.eulerAngles.z;
      if (blockAngleWorldSpace < 0.0f) {
        blockAngleWorldSpace += 360.0f;
      }

      if (blockAngleWorldSpace > 75.0f) {
        blockPatternData_[blockID].BlockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData_[blockID].BlockLightsLocalSpace);
      }

      if (blockAngleWorldSpace > 165.0f) {
        blockPatternData_[blockID].BlockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData_[blockID].BlockLightsLocalSpace);
      }

      if (blockAngleWorldSpace > 255.0f) {
        blockPatternData_[blockID].BlockLightsLocalSpace = BlockLights.GetRotatedClockwise(blockPatternData_[blockID].BlockLightsLocalSpace);
      }
    }

    private void HandleBlockMoving(int blockID, float xAccel, float yAccel, float zAccel) {
      blockPatternData_[blockID].Moving = true;
      blockPatternData_[blockID].LastFrameZAccel = zAccel;
      blockPatternData_[blockID].LastTimeTouched = Time.time;
      patternPlayAutoBuild_.ObjectMoved(blockID);
    }


    private void HandleBlockStopped(int blockID) {
      blockPatternData_[blockID].Moving = false;
    }

    private void SeenAccumulatorCompute() {
      foreach (KeyValuePair<int, BlockPatternData> kvp in blockPatternData_) {
        bool isVisible = false;
        for (int i = 0; i < CurrentRobot.VisibleObjects.Count; ++i) {
          if (CurrentRobot.VisibleObjects[i].ID == kvp.Key) {
            isVisible = true;
            break;
          }
        }
        if (isVisible) {
          kvp.Value.SeenAccumulator = Mathf.Min(1.5f, kvp.Value.SeenAccumulator + Time.deltaTime);
        }
        else {
          kvp.Value.SeenAccumulator = Mathf.Max(0.0f, kvp.Value.SeenAccumulator - Time.deltaTime * 2.0f);
        }

      }

    }

    private void PhoneCycle() {
      if (Input.GetMouseButtonDown(0)) {
        int lastTouchedID = SelectNewInputCandidate();

        if (lastTouchedID != -1) {
          blockPatternData_[lastTouchedID].BlockLightsLocalSpace = BlockLights.GetNextConfig(blockPatternData_[lastTouchedID].BlockLightsLocalSpace);
          blockPatternData_[lastTouchedID].LastTimeTouched = Time.time;
          patternPlayAudio_.PlayLightsSound(blockPatternData_[lastTouchedID].BlockLightsLocalSpace.NumberOfLightsOn());
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

        for (int i = 0; i < CurrentRobot.LightCubes[blockConfig.Key].Lights.Length; ++i) {
          CurrentRobot.LightCubes[blockConfig.Key].Lights[i].OnColor = CozmoPalette.ColorToUInt(disabledColor);
        }

        if (blockConfig.Key == currentInputID && blockPatternData_[blockConfig.Key].Moving && Time.time - lastSetTime_ > 5.0f) {
          enabledColor = Color.green;
          disabledColor = Color.green;
        }

        for (int i = 0; i < 4; ++i) {
          CurrentRobot.LightCubes[blockConfig.Key].Lights[i].OnColor = CozmoPalette.ColorToUInt(disabledColor);
        }

        if (blockConfig.Value.BlockLightsLocalSpace.back) {
          CurrentRobot.LightCubes[blockConfig.Key].Lights[1].OnColor = CozmoPalette.ColorToUInt(enabledColor);
        }

        if (blockConfig.Value.BlockLightsLocalSpace.front) {
          CurrentRobot.LightCubes[blockConfig.Key].Lights[3].OnColor = CozmoPalette.ColorToUInt(enabledColor);
        }

        if (blockConfig.Value.BlockLightsLocalSpace.left) {
          CurrentRobot.LightCubes[blockConfig.Key].Lights[2].OnColor = CozmoPalette.ColorToUInt(enabledColor);
        }

        if (blockConfig.Value.BlockLightsLocalSpace.right) {
          CurrentRobot.LightCubes[blockConfig.Key].Lights[0].OnColor = CozmoPalette.ColorToUInt(enabledColor);
        }

        // if cozmo is building his own pattern, then set the "seen" non-dirty blocks that are not
        // in a pattern yet to white.
        bool autoBuilding = patternPlayAutoBuild_.autoBuilding;
        bool nonDirtySeen = CurrentRobot.SeenObjects.Contains(CurrentRobot.LightCubes[blockConfig.Key]);
        bool notInNeatList = patternPlayAutoBuild_.NeatListContains(CurrentRobot.LightCubes[blockConfig.Key]) == false;
        bool notCarrying = true;
        if (patternPlayAutoBuild_.GetHeldObject() != null) {
          if (blockConfig.Key != patternPlayAutoBuild_.GetHeldObject().ID) {
            notCarrying = false;
          }
        }

        if (autoBuilding && nonDirtySeen && notInNeatList && notCarrying) {
          for (int i = 0; i < CurrentRobot.LightCubes[blockConfig.Key].Lights.Length; ++i) {
            CurrentRobot.LightCubes[blockConfig.Key].Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
          }
        }

      }

      previousInputID_ = currentInputID;
    }

    private void DetectPatterns() {
      // check cozmo vision for patterns.
      BlockPattern currentPattern = null;
      seenPattern_ = false;

      if (BlockPattern.ValidPatternSeen(out currentPattern, CurrentRobot, blockPatternData_)) {
        seenPattern_ = true;
        lastPatternSeen_ = Time.time;
        if (patternMemory_.AddSeen(currentPattern)) {
          shouldCelebrateNew_ = true;
        }
      }
    }

    private int SelectNewInputCandidate() {
      int lastTouchedID = -1;
      float minTime = -float.MaxValue;
      foreach (KeyValuePair<int, BlockPatternData> block in blockPatternData_) {
        if (block.Value.LastTimeTouched > minTime) {
          lastTouchedID = block.Key;
          minTime = block.Value.LastTimeTouched;
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
            kvp.Value.BlockLightsLocalSpace = BlockLights.GetNextConfig(kvp.Value.BlockLightsLocalSpace);
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

}
