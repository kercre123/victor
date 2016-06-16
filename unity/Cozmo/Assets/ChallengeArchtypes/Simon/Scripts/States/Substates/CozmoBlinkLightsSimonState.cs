using System;
using UnityEngine;

namespace Simon {
  public class CozmoBlinkLightsSimonState : State {
    private LightCube _TargetCube;
    private string _AnimGroupName;

    public CozmoBlinkLightsSimonState(LightCube targetCube, string animGroupName = null) {
      _TargetCube = targetCube;
      _AnimGroupName = animGroupName;
      if (_AnimGroupName == null) {
        _AnimGroupName = AnimationManager.Instance.GetAnimGroupForEvent(Anki.Cozmo.GameEvent.OnSimonPointCube);
      }
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      RobotEngineManager.Instance.OnRobotAnimationEvent += HandleRobotAnimationEvent;

      // Animation needs to have a robot event
      _CurrentRobot.SendAnimationGroup(_AnimGroupName, HandleEndAnimationComplete);
    }

    public override void Exit() {
      ResetLights();

      RobotEngineManager.Instance.OnRobotAnimationEvent -= HandleRobotAnimationEvent;
    }

    private void HandleRobotAnimationEvent(Anki.Cozmo.ExternalInterface.AnimationEvent animEvent) {
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

