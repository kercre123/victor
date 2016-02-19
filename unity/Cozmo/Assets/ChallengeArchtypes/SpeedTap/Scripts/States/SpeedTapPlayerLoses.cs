using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerLoses : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetFlashingLEDs(Color.red, 100, 100, 0);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.GotoPose(_SpeedTapGame.PlayPos, _CurrentRobot.Rotation, false, false, HandleAdjustDone);
      _SpeedTapGame.PlayerLosesHand();
    }

    private void HandleAdjustDone(bool success) {
      _CurrentRobot.SendAnimation(AnimationName.kSuccess, HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
    }
  }

}
