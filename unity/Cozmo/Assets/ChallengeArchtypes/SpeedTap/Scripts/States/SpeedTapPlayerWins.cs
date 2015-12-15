using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerWins : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetFlashingLEDs(Color.white, 100, 100, 0);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.SendAnimation(AnimationName.kFinishTabCubeLose, HandleAnimationDone);
      _SpeedTapGame.PlayerScore++;
      _SpeedTapGame.UpdateUI();
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
    }
  }

}
