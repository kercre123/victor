using UnityEngine;
using System.Collections;

public class SteerState : State {

  private float _LeftSpeed = 0.0f;
  private float _RightSpeed = 0.0f;
  private float _Duration = 0.0f;
  private State _NextState;
  private float _StartTime;

  public void Init(float wavePositionX, float duration, State nextState) {
    wavePositionX = 0.5f * wavePositionX + 0.5f;
    _LeftSpeed = Mathf.Lerp(35.0f, 25.0f, wavePositionX);
    _RightSpeed = Mathf.Lerp(25.0f, 35.0f, wavePositionX);

    _Duration = duration;
    _NextState = nextState;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.DriveWheels(_LeftSpeed, _RightSpeed);
    _StartTime = Time.time;
  }

  public override void Update() {
    base.Update();
    if (Time.time - _StartTime > _Duration) {
      _StateMachine.SetNextState(_NextState);
    }
  }

  public override void Exit() {
    base.Exit();
    _CurrentRobot.DriveWheels(0.0f, 0.0f);
  }
}
