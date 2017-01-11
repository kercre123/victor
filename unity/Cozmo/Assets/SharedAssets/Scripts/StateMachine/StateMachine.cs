using UnityEngine;
using System.Collections.Generic;

public class StateMachine {

  private List<State> _StateStack = new List<State>();

  private State _CurrState = null;
  private State _NextState = null;

  public State GetCurrState() {
    return _CurrState;
  }

  public State GetNextState() {
    return _NextState;
  }

  private GameBase _Game = null;

  private bool _IsPaused = false;

  private Anki.Cozmo.ReactionTrigger _reactionThatPausedGame = Anki.Cozmo.ReactionTrigger.NoneTrigger;

  public bool IsPaused {
    get {
      return _IsPaused;
    }
  }

  public GameBase GetGame() {
    return _Game;
  }

  public State GetParentState() {
    return _StateStack.Count > 0 ? _StateStack[_StateStack.Count - 1] : null;
  }

  // This is useful for if you want a reference back to the MonoBehaviour that created
  // and updates your state machine. You should be using GameBase as a common access
  // point for shared data across states.
  public void SetGameRef(GameBase game) {
    _Game = game;
  }

  public void SetNextState(State nextState) {
    _NextState = nextState;
    if (_NextState != null) {
      _NextState.SetStateMachine(this);
    }
  }

  public void PushSubState(State subState) {
    _StateStack.Add(_CurrState);
    _NextState = subState;
    _NextState.SetStateMachine(this);
    _CurrState = null;
  }

  public void PopState() {
    var oldState = _CurrState;

    _CurrState = _StateStack[_StateStack.Count - 1];
    _StateStack.RemoveAt(_StateStack.Count - 1);
    _NextState = null;

    if (oldState != null) {
      oldState.Exit();
    }
  }

  public void UpdateStateMachine() {
    if (_IsPaused) {
      if (_CurrState != null) {
        _CurrState.PausedUpdate();
      }
    }
    else {
      if (_NextState != null) {
        ProgressToNextState();
      }
      else if (_CurrState != null) {
        _CurrState.Update();
      }
    }
  }

  private void ProgressToNextState() {
    if (_CurrState != null) {
      if (_IsPaused) {
        _CurrState.Resume(State.PauseReason.ENGINE_MESSAGE, Anki.Cozmo.ReactionTrigger.Count);
      }
      _CurrState.Exit();
    }
    _CurrState = _NextState;
    _NextState = null;
    // its possible the call to CurrState.Exit
    // above could change the value of NextState to null
    if (_CurrState != null) {
      _CurrState.Enter();
    }
  }

  public void Stop() {
    if (_CurrState != null) {
      _CurrState.Exit();
      _CurrState = null;
    }
    _NextState = null;
  }

  public void Pause(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    if (!_IsPaused) {
      _IsPaused = true;
      if (_CurrState != null) {
        _reactionThatPausedGame = reactionaryBehavior;
        _CurrState.Pause(reason, reactionaryBehavior);
      }
    }
  }

  public void Resume(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    if (_IsPaused) {
      _IsPaused = false;
      if (_CurrState != null) {
        _reactionThatPausedGame = Anki.Cozmo.ReactionTrigger.NoneTrigger;
        _CurrState.Resume(reason, reactionaryBehavior);
      }
    }
  }

  public Anki.Cozmo.ReactionTrigger GetReactionThatPausedGame() {
    return _reactionThatPausedGame;
  }
}
