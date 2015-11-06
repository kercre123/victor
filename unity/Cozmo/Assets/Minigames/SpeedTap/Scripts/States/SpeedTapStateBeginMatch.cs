using System;

public class SpeedTapStateBeginMatch : State {

  private SpeedTapGame speedTapGame_ = null;

  public override void Enter() {
    base.Enter();
    speedTapGame_ = stateMachine_.GetGame() as SpeedTapGame;
    // Do a little dance
    // Blink some lights
    // Clear the score
    speedTapGame_.cozmoScore_ = 0;
    speedTapGame_.playerScore_ = 0;
    speedTapGame_.UpdateUI();
  }

  public override void Update() {
    base.Update();
    stateMachine_.SetNextState(new SpeedTapStatePlayNewHand());
  }
}


