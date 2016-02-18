using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private LightCube _TargetCube;
    private float _StartLightBlinkTime;
    private uint _CubeLightColor;
    private bool _IsAnimating = true;

    public CozmoBlinkLightsSimonState(LightCube targetCube) {
      _TargetCube = targetCube;
    }

    public override void Enter() {
      base.Enter();
      SetSimonNodeBlink();
    }

    public override void Update() {
      base.Update();
      if (Time.time - _StartLightBlinkTime > SimonGame.kLightBlinkLengthSeconds) {
        StopSimonNodeBlink();
        if (!_IsAnimating) {
          // Leave blink light state
          _StateMachine.PopState();
        }
      }
    }

    public override void Exit() {
      ResetLights();
    }

    private void SetSimonNodeBlink() {
      SimonGame game = _StateMachine.GetGame() as SimonGame;
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _StartLightBlinkTime = Time.time;
      _CubeLightColor = _TargetCube.Lights[0].OnColor;
      _TargetCube.TurnLEDsOff();
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(game.GetAudioForBlock(_TargetCube.ID));

      _CurrentRobot.SendAnimation(AnimationName.kSimonBlinkCube, HandleAnimationEnd);
      _IsAnimating = true;
    }

    private void HandleAnimationEnd(bool success) {
      // Leave animation state
      _IsAnimating = false;
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

