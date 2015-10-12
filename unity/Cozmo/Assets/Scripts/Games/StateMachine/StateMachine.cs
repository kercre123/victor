using UnityEngine;
using System.Collections;

public class StateMachine {

  State currState = null;
  State nextState = null;

  public void SetNextState(State nextState_) {
    nextState = nextState_;
    nextState.SetStateMachine(this);
  }

  public void UpdateStateMachine() {
    if (nextState != null) {
      currState.Exit();
      nextState.Enter();
      currState = nextState;
      nextState = null;
    }
    else {
      currState.Update();
    }
  }

}
  