using System;
using UnityEngine;

namespace Simon {
  public class TurnToTargetState : State  {

    private CozmoSetSimonState _Parent;

    public override void Enter() {
      base.Enter();
      _Parent = (CozmoSetSimonState)_StateMachine.GetParentState();
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    public override void Update() {
      base.Update();

      if (TurnToTarget()) {
        _StateMachine.SetNextState(new BlinkLightsState());
      }
    }

    private bool TurnToTarget() {
      LightCube currentTarget = _Parent.GetCurrentTarget();
      Vector3 robotToTarget = currentTarget.WorldPosition - _CurrentRobot.WorldPosition;
      float crossValue = Vector3.Cross(_CurrentRobot.Forward, robotToTarget).z;
      if (crossValue > 0.0f) {
        _CurrentRobot.DriveWheels(-35.0f, 35.0f);
      }
      else {
        _CurrentRobot.DriveWheels(35.0f, -35.0f);
      }
      return Vector2.Dot(robotToTarget.normalized, _CurrentRobot.Forward) > 0.98f;
    }
  }
}

