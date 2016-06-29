using UnityEngine;
using System.Collections;

public class State {

  protected StateMachine _StateMachine;

  protected IRobot _CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

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

  public virtual void Pause() {
    if (!DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
      // Show an alert view that quits the game
      _StateMachine.GetGame().ShowDontMoveCozmoAlertView();
    }
  }

  public virtual void Resume() {
    // Do nothing
  }
}
