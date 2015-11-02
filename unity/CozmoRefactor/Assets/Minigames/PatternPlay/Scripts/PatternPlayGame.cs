using UnityEngine;
using System.Collections;

public class PatternPlayGame : GameBase {

  StateMachineManager patternPlayStateMachineManager_ = new StateMachineManager();
  StateMachine patternPlayStateMachine_ = new StateMachine();

  void Start() {
    patternPlayStateMachine_.SetGameRef(this);
    patternPlayStateMachineManager_.AddStateMachine("PatternPlayStateMachine", patternPlayStateMachine_);
  }

  void Update() {

  }
}
