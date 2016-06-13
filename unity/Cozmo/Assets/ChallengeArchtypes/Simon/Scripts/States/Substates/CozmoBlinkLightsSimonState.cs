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

      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonPointCube, HandleEndAnimationComplete);
      // Animation needs to have a robot event
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonPointCube);
      RobotEngineManager.Instance.OnRobotAnimationEvent += OnRobotAnimationEvent;
    }

    public override void Exit() {
      ResetLights();

      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonPointCube, HandleEndAnimationComplete);
      RobotEngineManager.Instance.OnRobotAnimationEvent -= OnRobotAnimationEvent;
    }

    private void OnRobotAnimationEvent(Anki.Cozmo.ExternalInterface.AnimationEvent animEvent) {
      if (animEvent.event_id == Anki.Cozmo.AnimEvent.DEVICE_AUDIO_TRIGGER ||
          animEvent.event_id == Anki.Cozmo.AnimEvent.TAPPED_BLOCK) {
        int cubeId = _TargetCube.ID;
        SimonGame game = (SimonGame)_StateMachine.GetGame();
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(game.GetAudioForBlock(cubeId));
        game.BlinkLight(cubeId, SimonGame.kLightBlinkLengthSeconds, Color.black, game.GetColorForBlock(cubeId));
      }
    }

    private void HandleEndAnimationComplete(bool success) {
      _StateMachine.PopState();
    }

    private void ResetLights() {
      _CurrentRobot.TurnOffAllBackpackBarLED();
    }
  }
}

