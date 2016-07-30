using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace Selfie {
  public class PickupCozmoState : State {

    private float _PickupTime;

    SelfieGame _SelfieGame;

    public override void Enter() {
      base.Enter();
      _SelfieGame = (SelfieGame)_StateMachine.GetGame();
      _SelfieGame.SharedMinigameView.ProgressBar.NumSegments = _SelfieGame.CountdownTimer;
    }

    public override void Update() {
      base.Update();

      if (_CurrentRobot.Status(Anki.Cozmo.RobotStatusFlag.IS_PICKED_UP)) {
        _PickupTime += Time.deltaTime;

        // flash screen white before photo
        if (_PickupTime > _SelfieGame.CountdownTimer - 0.1f) {
          _SelfieGame.PrepareForPhoto();
        }
        if (_PickupTime > _SelfieGame.CountdownTimer) {
          GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Cozmo_Connect);
          _SelfieGame.TakePhoto();
          _StateMachine.SetNextState(new AnimationGroupState(Anki.Cozmo.AnimationTrigger.MajorWin, HandleAnimationDone));
        }
      }
      else {
        _PickupTime = 0f;
      }
      _SelfieGame.SharedMinigameView.ProgressBar.Progress = (Mathf.Clamp01(_PickupTime / _SelfieGame.CountdownTimer));


    }

    private void HandleAnimationDone(bool success) {
      _SelfieGame.StartPointlessGameEnd(true);
    }

  }
}
