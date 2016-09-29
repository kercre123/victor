using Cozmo.UI;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      if (_SpeedTapGame.GetPlayerBlock() == null) {
        foreach (var kvp in _CurrentRobot.LightCubes) {
          if (kvp.Value.ID != _SpeedTapGame.CozmoBlockID) {
            _SpeedTapGame.StartCycleCube(kvp.Value.ID,
              CubePalette.Instance.TapMeColor.lightColors,
              CubePalette.Instance.TapMeColor.cycleIntervalSeconds);
          }
        }
        _SpeedTapGame.ShowPlayerTapConfirmSlide();
      }
      else {
        _SpeedTapGame.StartCycleCube(_SpeedTapGame.PlayerBlockID,
          CubePalette.Instance.TapMeColor.lightColors,
          CubePalette.Instance.TapMeColor.cycleIntervalSeconds);
        _SpeedTapGame.ShowPlayerTapNewRoundSlide();
      }

      LightCube.TappedAction += HandleTap;
    }

    private void HandleTap(int id, int tappedTimes, float timeStamp) {
      if (_SpeedTapGame.PlayerBlockID == -1) {
        if (id != _SpeedTapGame.CozmoBlockID) {
          foreach (var kvp in _CurrentRobot.LightCubes) {
            if (kvp.Value.ID != _SpeedTapGame.CozmoBlockID) {
              _SpeedTapGame.StopCycleCube(kvp.Value.ID);
              kvp.Value.SetLEDsOff();
            }
          }
          _SpeedTapGame.SetPlayerCube(_CurrentRobot.LightCubes[id]);
          HandlePlayerCubeTap();
        }
      }
      else if (_SpeedTapGame.PlayerBlockID == id) {
        _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlockID);
        _CurrentRobot.LightCubes[_SpeedTapGame.PlayerBlockID].SetLEDsOff();
        HandlePlayerCubeTap();
      }
    }

    private void HandlePlayerCubeTap() {
      _StateMachine.SetNextState(new SpeedTapBeginRound());
    }

    public override void Exit() {
      base.Exit();
      _SpeedTapGame.SharedMinigameView.HideMiddleBackground();
      LightCube.TappedAction -= HandleTap;
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }

      _SpeedTapGame.SetLEDs(_SpeedTapGame.CozmoBlockID, Color.black);
      _SpeedTapGame.SetLEDs(_SpeedTapGame.PlayerBlockID, Color.black);
    }
  }
}
