using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerConfirm : State {

    private SpeedTapGame _SpeedTapGame = null;
    private const float _kCycleInterval = 1.25f;
    private float _CycleTimer = 0.0f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _SpeedTapGame.PlayerBlock.Lights[0].OnColor = Color.red.ToUInt();
      _SpeedTapGame.PlayerBlock.Lights[1].OnColor = Color.green.ToUInt();
      _SpeedTapGame.PlayerBlock.Lights[2].OnColor = Color.blue.ToUInt();
      _SpeedTapGame.PlayerBlock.Lights[3].OnColor = Color.yellow.ToUInt();
      _CycleTimer = _kCycleInterval;

      LightCube.TappedAction += HandleTap;
      _SpeedTapGame.ShowPlayerTapSlide();

    }

    public override void Update() {
      base.Update();
      if (_CycleTimer > 0.0f) {
        _CycleTimer -= Time.deltaTime;
      }
      else {
        _CycleTimer = _kCycleInterval;
        _SpeedTapGame.SpinLights(_SpeedTapGame.PlayerBlock);
      }
    }

    private void HandleTap(int id, int tappedTimes) {
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
