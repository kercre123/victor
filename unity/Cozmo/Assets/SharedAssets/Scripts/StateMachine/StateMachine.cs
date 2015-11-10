using UnityEngine;
using System.Collections;

public class StateMachine {

  private State _CurrState = null;
  private State _NextState = null;

  private GameBase _Game = null;

  public GameBase GetGame() {
    return _Game;
  }

  // This is useful for if you want a reference back to the MonoBehaviour that created
  // and updates your state machine. You should be using GameBase as a common access
  // point for shared data across states.
  public void SetGameRef(GameBase game) {
    _Game = game;
  }

  public void SetNextState(State nextState) {
    _NextState = nextState;
    _NextState.SetStateMachine(this);
  }

  public void UpdateStateMachine() {
    if (_NextState != null) {
      if (_CurrState != null) {
        _CurrState.Exit();
      }
      _NextState.Enter();
      _CurrState = _NextState;
      _NextState = null;
    }
    else {
      _CurrState.Update();
    }
  }

}
  