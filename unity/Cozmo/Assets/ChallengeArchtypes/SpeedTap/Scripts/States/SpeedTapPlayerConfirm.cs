using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _SpeedTapGame.StartCycleCube(_SpeedTapGame.PlayerBlock, 
        Cozmo.CubePalette.TapMeColor.lightColors, 
        Cozmo.CubePalette.TapMeColor.cycleIntervalSeconds);

      LightCube.TappedAction += HandleTap;
      _SpeedTapGame.ShowPlayerTapSlide();
    }

    private void HandleTap(int id, int tappedTimes) {
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlock);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapWin);
      if (id == _SpeedTapGame.PlayerBlock.ID) {
        _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      }
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      LightCube.TappedAction -= HandleTap;
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.ResetScore();
    }
  }

}
