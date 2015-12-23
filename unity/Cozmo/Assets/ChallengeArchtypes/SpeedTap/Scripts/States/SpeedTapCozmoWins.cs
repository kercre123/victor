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
      _SpeedTapGame.CozmoBlock.SetFlashingLEDs(_SpeedTapGame.MatchColor, 100, 100, 0);
      _CurrentRobot.SendAnimation(AnimationName.kFinishTapCubeWin, HandleAnimationDone);
    }

    private void HandleAnimationDone(bool success) {
      _SpeedTapGame.PlayerBlock.SetLEDs(Color.black);
      _SpeedTapGame.CozmoBlock.SetLEDs(Color.black);
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
      _SpeedTapGame.CozmoWinsHand();
    }
  }

}
