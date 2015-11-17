using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class StateMachineManager {

  Dictionary<string, StateMachine> _StateMachines = new Dictionary<string, StateMachine>();

  public void AddStateMachine(string stateMachineName, StateMachine stateMachine) {
    if (_StateMachines.ContainsKey(stateMachineName)) {
      DAS.Error(this, "Duplicate State Machine Name");
    }
    _StateMachines.Add(stateMachineName, stateMachine);
  }

  public bool RemoveStateMachine(string stateMachineName) {
    return _StateMachines.Remove(stateMachineName);
  }

  public void UpdateAllMachines() {
    foreach (KeyValuePair<string, StateMachine> kvp in _StateMachines) {
      kvp.Value.UpdateStateMachine();
    }
  }
}
