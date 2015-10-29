using System;

public class SpeedTapStateBeginMatch : State {

  SpeedTapController speedTapController = null;

  public override void Enter() {
    base.Enter();
    speedTapController = stateMachine.GetGameController() as SpeedTapController;
    // Do a little dance
    // Blink some lights
    // Clear the score
    speedTapController.cozmoScore = 0;
    speedTapController.playerScore = 0;
    speedTapController.UpdateUI();
  }

  public override void Update() {
    base.Update();
    stateMachine.SetNextState(new SpeedTapStatePlayNewHand());
  }
}


