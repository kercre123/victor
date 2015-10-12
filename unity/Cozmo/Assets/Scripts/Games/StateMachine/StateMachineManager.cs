using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class StateMachineManager : MonoBehaviour {

  List<StateMachine> stateMachines = new List<StateMachine>();

  public void AddStateMachine(StateMachine stateMachine) {
    stateMachines.Add(stateMachine);
  }

  void Update() {
    for (int i = 0; i < stateMachines.Count; ++i) {
      stateMachines[i].UpdateStateMachine();
    }
  }
}
