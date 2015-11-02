using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternPlayGame : GameBase {

  StateMachineManager patternPlayStateMachineManager_ = new StateMachineManager();
  StateMachine patternPlayStateMachine_ = new StateMachine();

  private Dictionary<int, BlockPatternData> blockPatternData = new Dictionary<int, BlockPatternData>();
  private PatternMemory memoryBank = new PatternMemory();

  void Start() {
    patternPlayStateMachine_.SetGameRef(this);
    patternPlayStateMachineManager_.AddStateMachine("PatternPlayStateMachine", patternPlayStateMachine_);
    InitialCubesState initCubeState = new InitialCubesState();
    initCubeState.InitialCubeRequirements(new LookForPatternState(), 3);
    patternPlayStateMachine_.SetNextState(initCubeState);
  }

  void Update() {

  }
}
