using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private LightCube _TargetCube;
    private Anki.Cozmo.AnimationTrigger _AnimGroupTrigger;
    private bool _IsCorrect = false;

    public CozmoBlinkLightsSimonState(LightCube targetCube, Anki.Cozmo.AnimationTrigger animGroupTrigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPointCenter, bool isCorrect = true) {
      _TargetCube = targetCube;
      _AnimGroupTrigger = animGroupTrigger;
      _IsCorrect = isCorrect;
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.AnimationEvent>(HandleRobotAnimationEvent);

      // Animation needs to have a robot event
      _CurrentRobot.SendAnimationTrigger(_AnimGroupTrigger, HandleEndAnimationComplete);
    }

    public override void Exit() {
      ResetLights();

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.AnimationEvent>(HandleRobotAnimationEvent);
    }

    private void HandleRobotAnimationEvent(Anki.Cozmo.ExternalInterface.AnimationEvent animEvent) {
      if (animEvent.event_id == Anki.Cozmo.AnimEvent.DEVICE_AUDIO_TRIGGER ||
          animEvent.event_id == Anki.Cozmo.AnimEvent.TAPPED_BLOCK) {
        int cubeId = _TargetCube.ID;
        SimonGame game = (SimonGame)_StateMachine.GetGame();
        if (_IsCorrect) {
          Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(game.GetAudioForBlock(cubeId));
        }
        else {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Lose);
        }
        game.BlinkLight(cubeId, SimonGame.kLightBlinkLengthSeconds, Color.black, game.GetColorForBlock(cubeId));
      }
    }

    private void HandleEndAnimationComplete(bool success) {
      _StateMachine.PopState();
    }

    private void ResetLights() {
      if (_CurrentRobot != null) {
        _CurrentRobot.TurnOffAllBackpackBarLED();
      }
    }
  }
}

