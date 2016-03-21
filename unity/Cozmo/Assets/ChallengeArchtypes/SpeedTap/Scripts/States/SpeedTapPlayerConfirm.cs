using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      if (_SpeedTapGame.PlayerBlock == null) {
        foreach (var kvp in _CurrentRobot.LightCubes) {
          if (kvp.Value.ID != _SpeedTapGame.CozmoBlock.ID) {
            _SpeedTapGame.StartCycleCube(kvp.Value.ID, 
              Cozmo.CubePalette.TapMeColor.lightColors, 
              Cozmo.CubePalette.TapMeColor.cycleIntervalSeconds);
          }
        }
        _SpeedTapGame.ShowPlayerTapConfirmSlide();
      }
      else {
        _SpeedTapGame.StartCycleCube(_SpeedTapGame.PlayerBlock.ID, 
          Cozmo.CubePalette.TapMeColor.lightColors, 
          Cozmo.CubePalette.TapMeColor.cycleIntervalSeconds);
        _SpeedTapGame.ShowPlayerTapNewRoundSlide();
      }

      LightCube.TappedAction += HandleTap;
    }

    private void HandleTap(int id, int tappedTimes) {
      if (_SpeedTapGame.PlayerBlock == null) {
        if (id != _SpeedTapGame.CozmoBlock.ID) {
          foreach (var kvp in _CurrentRobot.LightCubes) {
            if (kvp.Value.ID != _SpeedTapGame.CozmoBlock.ID) {
              _SpeedTapGame.StopCycleCube(kvp.Value.ID);
              kvp.Value.TurnLEDsOff();
            }
          }
          _SpeedTapGame.SetPlayerCube(_CurrentRobot.LightCubes[id]);
          HandlePlayerCubeTap();
        }
      }
      else if (_SpeedTapGame.PlayerBlock.ID == id) {
        _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlock.ID);
        _SpeedTapGame.PlayerBlock.TurnLEDsOff();
        HandlePlayerCubeTap();
      }
    }

    private void HandlePlayerCubeTap() {
      // Ben wants to use the same sound for player tap and for Cozmo tap
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CubeCozmoTap);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      LightCube.TappedAction -= HandleTap;
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);

      if (_SpeedTapGame.PlayerBlock != null) {
        _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      }

      _SpeedTapGame.ResetScore();
    }
  }

}
