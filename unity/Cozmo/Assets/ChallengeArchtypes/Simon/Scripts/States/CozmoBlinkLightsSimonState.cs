using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private const float kLightBlinkLengthSeconds = 0.3f;
    private LightCube _TargetCube;
    private float _StartLightBlinkTime;
    private uint _CubeLightColor;

    public CozmoBlinkLightsSimonState(LightCube targetCube) {
      _TargetCube = targetCube;
    }

    public override void Enter() {
      base.Enter();
      SetSimonNodeBlink();
    }

    public override void Update() {
      base.Update();
      if (Time.time - _StartLightBlinkTime > kLightBlinkLengthSeconds) {
        StopSimonNodeBlink();
      }
    }

    public override void Exit() {
      ResetLights();
    }

    private void SetSimonNodeBlink() {
      SimonGame game = _StateMachine.GetGame() as SimonGame;
      string animation = game.GetCozmoAnimationForBlock(_TargetCube.ID);
      _CurrentRobot.SendAnimation(animation, HandleAnimationEnd);
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _StartLightBlinkTime = Time.time;
      _CubeLightColor = _TargetCube.Lights[0].OnColor;
      _TargetCube.TurnLEDsOff();
      _CurrentRobot.SetAllBackpackBarLED(_CubeLightColor);
    }

    private void HandleAnimationEnd(bool success) {
      _StateMachine.PopState();
    }

    private void StopSimonNodeBlink() {
      ResetLights();
    }

    private void ResetLights() {
      _TargetCube.SetLEDs(_CubeLightColor, 0, uint.MaxValue, 0, 0, 0);
      _CurrentRobot.TurnOffAllBackpackBarLED();
    }
  }
}

