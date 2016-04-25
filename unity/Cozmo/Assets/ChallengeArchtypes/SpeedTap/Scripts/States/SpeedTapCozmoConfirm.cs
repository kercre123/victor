using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapCozmoConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapCozmoConfirm, HandleTapDone);
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapCozmoConfirm);
      if (_SpeedTapGame.PlayerBlock != null) {
        _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlock);
        _SpeedTapGame.PlayerBlock.SetLEDsOff();
      }
    }

    private void HandleTapDone(bool success) {
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapCozmoConfirm, HandleTapDone);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CubeCozmoTap);
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.CozmoBlock);
      _SpeedTapGame.CozmoBlock.SetLEDs(Cozmo.CubePalette.ReadyColor.lightColor);
      _SpeedTapGame.SetCozmoOrigPos();
      _StateMachine.SetNextState(new SpeedTapPlayerConfirm());
    }
  }

}
