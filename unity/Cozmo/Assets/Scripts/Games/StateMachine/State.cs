using UnityEngine;
using System.Collections;

public class State {

  protected StateMachine stateMachine;

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public void SetStateMachine(StateMachine stateMachine_) {
    stateMachine = stateMachine_;
  }

  public virtual void Enter() {
    
  }

  public virtual void Update() {
    
  }

  public virtual void Exit() {

  }

}
