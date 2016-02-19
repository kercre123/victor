using System;
using UnityEngine;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapCozmoWins : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      for (int i = 0; i < 4; i++) {
        _SpeedTapGame.CozmoBlock.Lights[i].SetFlashingLED(_SpeedTapGame.CozmoWinColors[i], 100, 100, 0);
      }
      _CurrentRobot.GotoPose(_SpeedTapGame.PlayPos, _SpeedTapGame.PlayRot, false, false, HandleAdjustDone);
    }

    private void HandleAdjustDone(bool success) {
      _CurrentRobot.SendAnimation(AnimationName.kSuccess, HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      _SpeedTapGame.CozmoWinsHand();
    }
  }

}
