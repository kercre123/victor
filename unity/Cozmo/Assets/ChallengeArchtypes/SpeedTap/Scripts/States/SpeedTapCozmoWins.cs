
using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapCozmoWins : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetFlashingLEDs(Color.red, 100, 100, 0);
      for (int i = 0; i < 4; i++) {
        _SpeedTapGame.CozmoBlock.Lights[i].SetFlashingLED(_SpeedTapGame.CozmoWinColors[i], 100, 100, 0);
      }
      _SpeedTapGame.CozmoAdjust();
    }

    public override void Update() {
      base.Update();
      if (_SpeedTapGame.CozmoAdjustTimeLeft > 0.0f) {
        _SpeedTapGame.CozmoAdjustTimeLeft -= Time.deltaTime;
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        _StateMachine.SetNextState(new AnimationState(_SpeedTapGame.RandomWinHand(), HandleAnimationDone));
      }
    }


    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      _SpeedTapGame.CozmoWinsHand();
    }
  }

}
