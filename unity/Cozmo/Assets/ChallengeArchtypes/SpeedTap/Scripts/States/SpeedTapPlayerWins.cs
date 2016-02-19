using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerWins : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      for (int i = 0; i < 4; i++) {
        _SpeedTapGame.PlayerBlock.Lights[i].SetFlashingLED(_SpeedTapGame.PlayerWinColors[i], 100, 100, 0);
      }
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.FaceObject(_SpeedTapGame.CozmoBlock, false, 4.3f, 10.0f, HandleAdjustDone);
    }

    private void HandleAdjustDone(bool success) {
      _CurrentRobot.SendAnimation(AnimationName.kFail, HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      _SpeedTapGame.PlayerWinsHand();
    }
  }

}
