using UnityEngine;
using System.Collections;

public class StateMachine {

  State currState = null;
  State nextState = null;

  GameController gameController = null;

  public GameController GetGameController() {
    return gameController;
  }

  public void SetGameController(GameController gameController_) {
    gameController = gameController_;
  }

  public void SetNextState(State nextState_) {
    nextState = nextState_;
    nextState.SetStateMachine(this);
  }

  public void UpdateStateMachine() {
    if (nextState != null) {
      if (currState != null) {
        currState.Exit();
      }
      nextState.Enter();
      currState = nextState;
      nextState = null;
    }
    else {
      currState.Update();
    }
  }

}
  