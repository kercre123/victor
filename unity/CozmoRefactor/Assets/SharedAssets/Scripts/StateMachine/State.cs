using UnityEngine;
using System.Collections;

public class State {

  protected StateMachine stateMachine_;

  public void SetStateMachine(StateMachine stateMachine) {
    stateMachine_ = stateMachine;
  }

  public virtual void Enter() {
    DAS.Info("State.Enter", this.GetType().ToString());
  }

  public virtual void Update() {
    // NO SPAM, don't log here
  }

  public virtual void Exit() {
    DAS.Info("State.Exit", this.GetType().ToString());
  }

}
