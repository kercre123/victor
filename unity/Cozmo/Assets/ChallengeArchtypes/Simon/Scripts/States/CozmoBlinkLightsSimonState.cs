using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private LightCube _TargetCube;
    private uint _CubeLightColor;

    public CozmoBlinkLightsSimonState(LightCube targetCube) {
      _TargetCube = targetCube;
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CubeLightColor = _TargetCube.Lights[0].OnColor;
      _CurrentRobot.SendAnimation(AnimationName.kSimonBlinkStart, HandleStartAnimationComplete, Anki.Cozmo.QueueActionPosition.NOW_AND_RESUME);
      _CurrentRobot.SendAnimation(AnimationName.kSimonBlinkEnd, HandleEndAnimationComplete, Anki.Cozmo.QueueActionPosition.AT_END);
    }
      
    public override void Exit() {
      ResetLights();
    }

    private void HandleStartAnimationComplete(bool success) {
      SimonGame game = (SimonGame)_StateMachine.GetGame();
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(game.GetAudioForBlock(_TargetCube.ID));

    }

    private void HandleEndAnimationComplete(bool success) {
      _StateMachine.PopState();
    }
      
    private void ResetLights() {
      _TargetCube.SetLEDs(_CubeLightColor, 0, Robot.Light.FOREVER, 0, 0, 0);
      _CurrentRobot.TurnOffAllBackpackBarLED();
    }
  }
}

