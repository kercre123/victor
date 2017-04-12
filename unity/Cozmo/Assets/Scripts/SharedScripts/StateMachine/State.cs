using UnityEngine;
using System.Collections;

public class State {

  public enum PauseReason {
    ENGINE_MESSAGE,
    DEBUG_INPUT,
    PLAYER_INPUT
  }
  protected StateMachine _StateMachine;

  protected IRobot _CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private bool _IsPauseable = true;

  public bool IsPauseable {
    get {
      return _IsPauseable;
    }
    protected set {
      _IsPauseable = value;
    }
  }

  public void SetStateMachine(StateMachine stateMachine) {
    _StateMachine = stateMachine;
  }

  public virtual void Enter() {
    DAS.Info("State.Enter", this.GetType().ToString());
  }

  public virtual void Update() {
    // NO SPAM, don't log here
  }

  public virtual void PausedUpdate() {
    // NO SPAM, don't log here
  }

  public virtual void Exit() {
    DAS.Info("State.Exit", this.GetType().ToString());
  }

  public virtual void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    ShowDontMoveCozmoView();
  }

  public virtual void Resume(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    // Do nothing
  }

  protected void ShowDontMoveCozmoView() {
    // Show an alert view that quits the game
    _StateMachine.GetGame().ShowDontMoveCozmoAlertView();
  }
}
