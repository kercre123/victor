using System;
using UnityEngine;

namespace Simon {
  public class CozmoTurnToCubeSimonState : State {
    public const float kDriveWheelSpeed = 80f;
    public const float kDotThreshold = 0.96f;

    private LightCube _TargetCube;
    private bool _BlinkLights = false;
    private bool _IsTurning = true;

    public CozmoTurnToCubeSimonState(LightCube targetCube, bool blinkLights) {
      _TargetCube = targetCube;
      _BlinkLights = blinkLights;
    }

    public override void Enter() {
      base.Enter();
      _IsTurning = true;
      _CurrentRobot.FaceObject(_TargetCube, false, HandleTurnFinished);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    public override void Update() {
      base.Update();

      if (!_IsTurning) {
        if (_BlinkLights) {
          _StateMachine.SetNextState(new CozmoBlinkLightsSimonState(_TargetCube));
        }
        else {
          _StateMachine.PopState();
        }
      }
    }

    private void HandleTurnFinished(bool success) {
      _IsTurning = false;
    }
  }
}

