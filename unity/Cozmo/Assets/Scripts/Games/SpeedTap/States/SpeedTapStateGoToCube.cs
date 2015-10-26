using System;


public class SpeedTapStateGoToCube : State {

  SpeedTapController speedTapController = null;

  public override void Enter() {
    base.Enter();
    speedTapController = stateMachine.GetGameController() as SpeedTapController;
    robot.SetLiftHeight(1.0f);
    robot.SetHeadAngle();
    robot.GotoObject(speedTapController.cozmoBlock, 50.0f, (bool success) => {
      if(success) {
        stateMachine.SetNextState(new SpeedTapStateBeginMatch());
      }
      else {
        // AAAAAAAAA!!
      }
    });
  }
}


