using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerWins : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.CozmoBlock.SetFlashingLEDs(Color.red, 100, 100, 0);
      for (int i = 0; i < 4; i++) {
        _SpeedTapGame.PlayerBlock.Lights[i].SetFlashingLED(_SpeedTapGame.PlayerWinColors[i], 100, 100, 0);
      }
      _SpeedTapGame.CheckForAdjust(HandleAdjustDone);
    }

    private void HandleAdjustDone(bool success) {
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_LoseHand, HandleAnimationDone));
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      _SpeedTapGame.PlayerWinsHand();
    }
  }

}
