using UnityEngine;
using System.Collections;

namespace DockTraining {
  public class SteerState : State {

    private float _LeftSpeed = 0.0f;
    private float _RightSpeed = 0.0f;
    private float _Duration = 0.0f;
    private State _NextState;
    private float _StartTime;

    public void Init(float leftSpeed, float rightSpeed, float duration, State nextState) {
      _LeftSpeed = leftSpeed;
      _RightSpeed = rightSpeed;
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
}