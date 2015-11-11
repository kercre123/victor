using UnityEngine;
using System.Collections;

public class State {

  protected StateMachine _StateMachine;

  protected Robot _CurrentRobot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }

  public void SetStateMachine(StateMachine stateMachine) {
    _StateMachine = stateMachine;
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
