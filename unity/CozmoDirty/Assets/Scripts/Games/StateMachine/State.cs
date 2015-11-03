using UnityEngine;
using System.Collections;

public class State {

  protected StateMachine stateMachine;

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public void SetStateMachine(StateMachine stateMachine_) {
    stateMachine = stateMachine_;
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
