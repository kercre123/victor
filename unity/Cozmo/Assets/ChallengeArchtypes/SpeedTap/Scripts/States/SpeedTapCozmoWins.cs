using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapCozmoWins : State {

    private SpeedTapGame _SpeedTapGame = null;
    private float _DriveTime = 0.5f;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      for (int i = 0; i < 4; i++) {
        _SpeedTapGame.CozmoBlock.Lights[i].SetFlashingLED(_SpeedTapGame.CozmoWinColors[i], 100, 100, 0);
      }
      _CurrentRobot.DriveWheels(-20.0f, -20.0f);
    }

    public override void Update() {
      base.Update();
      _DriveTime -= Time.deltaTime;
      if (_DriveTime < 0.0f) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kSpeedTap_Tap_01, HandleAdjustDone));
      }
    }

    private void HandleAdjustDone(bool success) {
      _CurrentRobot.SendAnimation(AnimationName.kSpeedTap_WinHand, HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      _SpeedTapGame.CozmoWinsHand();
    }
  }

}
