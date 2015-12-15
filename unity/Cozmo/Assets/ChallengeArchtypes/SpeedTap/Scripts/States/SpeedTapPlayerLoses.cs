using System;
using UnityEngine;

namespace SpeedTap {

  public class SpeedTapPlayerLoses : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.red);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _CurrentRobot.SendAnimation(AnimationName.kFinishTapCubeWin, HandleAnimationDone);
      _SpeedTapGame.PlayerScore = Mathf.Max(0, _SpeedTapGame.PlayerScore - 1);
      _SpeedTapGame.UpdateUI();
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
    }
  }

}
