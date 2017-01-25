using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSpeedtapCozmoConfirm, HandleTapDone);
    }

    public override void Exit() {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetEnableCliffSensor(false);
      }
    }

    private void HandleTapDone(bool success) {
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Cube_Cozmo_Tap);
      _SpeedTapGame.SetLEDs(_SpeedTapGame.CozmoBlockID, Cozmo.UI.CubePalette.Instance.ReadyColor.lightColor);
      _SpeedTapGame.SetCozmoOrigPos();
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
    }
  }

}
