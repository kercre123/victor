using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class StateMachineManager : MonoBehaviour {

  Dictionary<string, StateMachine> stateMachines = new Dictionary<string, StateMachine>();

  public void AddStateMachine(string stateMachineName, StateMachine stateMachine) {
    if (stateMachines.ContainsKey(stateMachineName)) {
      DAS.Error("StateMachineManager", "Duplicate State Machine Name");
    }
    stateMachines.Add(stateMachineName, stateMachine);
  }

  public bool RemoveStateMachine(string stateMachineName) {
    return stateMachines.Remove(stateMachineName);
  }

  void Update() {
    foreach (KeyValuePair<string, StateMachine> kvp in stateMachines) {
      kvp.Value.UpdateStateMachine();
    }
  }
}
