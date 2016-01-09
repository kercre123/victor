using System;
using UnityEngine;

namespace Simon {
  public class CozmoTurnToCubeSimonState : State {
    public const float kDriveWheelSpeed = 80f;
    public const float kDotThreshold = 0.96f;

    private LightCube _TargetCube;
    private bool _BlinkLights = false;

    public CozmoTurnToCubeSimonState(LightCube targetCube, bool blinkLights) {
      _TargetCube = targetCube;
      _BlinkLights = blinkLights;
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    public override void Update() {
      base.Update();

      if (TurnToTarget(_TargetCube)) {
        if (_BlinkLights) {
          _StateMachine.SetNextState(new CozmoBlinkLightsSimonState(_TargetCube));
        }
        else {
          _StateMachine.PopState();
        }
      }
    }

    private bool TurnToTarget(LightCube target) {
      Robot robot = _CurrentRobot;
      Vector3 robotToTarget = target.WorldPosition - robot.WorldPosition;
      float crossValue = Vector3.Cross(robot.Forward, robotToTarget).z;
      if (crossValue > 0.0f) {
        robot.DriveWheels(-kDriveWheelSpeed, kDriveWheelSpeed);
      }
      else {
        robot.DriveWheels(kDriveWheelSpeed, -kDriveWheelSpeed);
      }
      return Vector2.Dot(robotToTarget.normalized, robot.Forward) > kDotThreshold;
    }
  }
}

