using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private LightCube _TargetCube;

    public CozmoBlinkLightsSimonState(LightCube targetCube) {
      _TargetCube = targetCube;
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SendAnimation(AnimationName.kSimonBlinkStart, HandleStartAnimationComplete, Anki.Cozmo.QueueActionPosition.NOW_AND_RESUME);
      _CurrentRobot.SendAnimation(AnimationName.kSimonBlinkEnd, HandleEndAnimationComplete, Anki.Cozmo.QueueActionPosition.AT_END);
    }

    public override void Exit() {
      ResetLights();
    }

    private void HandleStartAnimationComplete(bool success) {
      int cubeId = _TargetCube.ID;
      SimonGame game = (SimonGame)_StateMachine.GetGame();
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(game.GetAudioForBlock(cubeId));
      game.BlinkLight(cubeId, SimonGame.kLightBlinkLengthSeconds, Color.black, game.GetColorForBlock(cubeId));
    }

    private void HandleEndAnimationComplete(bool success) {
      _StateMachine.PopState();
    }

    private void ResetLights() {
      _CurrentRobot.TurnOffAllBackpackBarLED();
    }
  }
}

