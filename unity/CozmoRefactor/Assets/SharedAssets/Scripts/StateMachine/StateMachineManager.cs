using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class StateMachineManager {

  Dictionary<string, StateMachine> stateMachines_ = new Dictionary<string, StateMachine>();

  public void AddStateMachine(string stateMachineName, StateMachine stateMachine) {
    if (stateMachines_.ContainsKey(stateMachineName)) {
      DAS.Error("StateMachineManager", "Duplicate State Machine Name");
    }
    stateMachines_.Add(stateMachineName, stateMachine);
  }

  public bool RemoveStateMachine(string stateMachineName) {
    return stateMachines_.Remove(stateMachineName);
  }

  public void UpdateAllMachines() {
    foreach (KeyValuePair<string, StateMachine> kvp in stateMachines_) {
      kvp.Value.UpdateStateMachine();
    }
  }
}
