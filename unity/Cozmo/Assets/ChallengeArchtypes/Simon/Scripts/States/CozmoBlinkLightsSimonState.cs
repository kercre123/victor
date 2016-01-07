using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private const float kLightBlinkLengthSeconds = 0.3f;
    private CozmoSetSequenceSimonState _Parent;
    private float _StartLightBlinkTime;
    private uint _CubeLightColor;

    public override void Enter() {
      base.Enter();
      _Parent = (CozmoSetSequenceSimonState)_StateMachine.GetParentState();
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
      LightCube currentCube = _Parent.GetCurrentTarget();
      SimonGame game = _StateMachine.GetGame() as SimonGame;
      string animation = game.GetCozmoAnimationForBlock(currentCube.ID);
      _CurrentRobot.SendAnimation(animation, HandleAnimationEnd);
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _StartLightBlinkTime = Time.time;
      _CubeLightColor = currentCube.Lights[0].OnColor;
      currentCube.TurnLEDsOff();
      _CurrentRobot.SetAllBackpackBarLED(_CubeLightColor);
    }

    private void HandleAnimationEnd(bool success) {
      _StateMachine.PopState();
    }

    private void StopSimonNodeBlink() {
      ResetLights();
    }

    private void ResetLights() {
      LightCube currentCube = _Parent.GetCurrentTarget();
      currentCube.SetLEDs(_CubeLightColor, 0, uint.MaxValue, 0, 0, 0);
      _CurrentRobot.TurnOffAllBackpackBarLED();
    }
  }
}

