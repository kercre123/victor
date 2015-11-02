using UnityEngine;
using System.Collections;

public class StateMachine {

  State currState_ = null;
  State nextState_ = null;

  GameBase game_ = null;

  public GameBase GetGame() {
    return game_;
  }

  // This is useful for if you want a reference back to the MonoBehaviour that created
  // and updates your state machine. You should be using GameBase as a common access
  // point for shared data across states.
  public void SetGameRef(GameBase game) {
    game_ = game;
  }

  public void SetNextState(State nextState) {
    nextState_ = nextState;
    nextState_.SetStateMachine(this);
  }

  public void UpdateStateMachine() {
    if (nextState_ != null) {
      if (currState_ != null) {
        currState_.Exit();
      }
      nextState_.Enter();
      currState_ = nextState_;
      nextState_ = null;
    }
    else {
      currState_.Update();
    }
  }

}
  