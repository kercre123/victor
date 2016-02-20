using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private LightCube _TargetCube;
    private float _StartLightBlinkTime;
    private uint _CubeLightColor;
    private bool _IsAnimating = true;
    private bool _LightsAreOff = false;

    public CozmoBlinkLightsSimonState(LightCube targetCube) {
      _TargetCube = targetCube;
    }

    public override void Enter() {
      base.Enter();
      SetSimonNodeBlink();
    }

    public override void Update() {
      base.Update();
      float secondsSinceAnimationStart = Time.time - _StartLightBlinkTime;
      if (secondsSinceAnimationStart > SimonGame.kLightBlinkLengthSeconds + SimonGame.kCozmoLightBlinkDelaySeconds) {
        StopSimonNodeBlink();
        if (!_IsAnimating) {
          // Leave blink light state
          _StateMachine.PopState();
        }
      }
      else if (secondsSinceAnimationStart > SimonGame.kCozmoLightBlinkDelaySeconds) {
        // Start the light blink when Cozmo lifts his lift during the animation
        // TODO: For now just use a hardcoded value but we need a better way to coordinate this with the animation
        TurnOffLights();
      }
    }

    public override void Exit() {
      ResetLights();
    }

    private void SetSimonNodeBlink() {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _StartLightBlinkTime = Time.time;
      _CubeLightColor = _TargetCube.Lights[0].OnColor;
      _LightsAreOff = false;

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

    private void TurnOffLights() {
      if (!_LightsAreOff) {
        SimonGame game = _StateMachine.GetGame() as SimonGame;
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(game.GetAudioForBlock(_TargetCube.ID));
        _TargetCube.TurnLEDsOff();
        _LightsAreOff = true;
      }
    }

    private void ResetLights() {
      _TargetCube.SetLEDs(_CubeLightColor, 0, uint.MaxValue, 0, 0, 0);
      _CurrentRobot.TurnOffAllBackpackBarLED();
    }
  }
}

