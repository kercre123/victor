using System;

namespace SpeedTap {

  public class SpeedTapStateBeginMatch : State {

    private SpeedTapGame _SpeedTapGame = null;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      // Do a little dance
      // Blink some lights
      // Clear the score
      _SpeedTapGame.CozmoScore = 0;
      _SpeedTapGame.PlayerScore = 0;
      _SpeedTapGame.UpdateUI();
    }

    public override void Update() {
      base.Update();
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
    }
  }

}
