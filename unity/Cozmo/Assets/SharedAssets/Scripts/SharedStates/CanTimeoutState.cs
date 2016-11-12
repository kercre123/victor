using UnityEngine;
using System.Collections;

// How long is a state willing to go without any user input before we interrupt them...
public class CanTimeoutState : State {

  private float _StartTime;
  private float _Duration;

  public CanTimeoutState(float duration = 60.0f) {
    _Duration = duration;
  }

  public override void Enter() {
    base.Enter();
    ResetPlayerInputTimer();
  }

  protected void ResetPlayerInputTimer() {
    _StartTime = Time.time;
  }
  protected void SetTimeoutDuration(float duration) {
    _Duration = duration;
  }

  public override void Update() {
    base.Update();
    if (Time.time - _StartTime > _Duration) {
      if (_StateMachine != null) {
        _StateMachine.GetGame().ShowInterruptionQuitGameView(LocalizationKeys.kMinigameTimeoutTitle,
            LocalizationKeys.kMinigameTimeoutDescription);
      }
    }
  }

}
