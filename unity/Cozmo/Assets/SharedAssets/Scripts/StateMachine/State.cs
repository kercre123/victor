using UnityEngine;
using System.Collections;

public class State {

  protected StateMachine stateMachine_;

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }

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
