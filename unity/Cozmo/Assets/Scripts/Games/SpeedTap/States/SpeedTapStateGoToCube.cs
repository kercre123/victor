using System;


public class SpeedTapStateGoToCube : State {

  SpeedTapController speedTapController = null;
  bool drivingToBlock = false;

  public override void Enter() {
    base.Enter();
    speedTapController = stateMachine.GetGameController() as SpeedTapController;
    robot.SetLiftHeight(1.0f);
    robot.SetHeadAngle();
  }

  void DriveToBlock() {
    if(drivingToBlock) return;
    if (speedTapController.cozmoBlock.MarkersVisible) {
      drivingToBlock = true;
      robot.GotoObject(speedTapController.cozmoBlock, 50.0f, (bool success) => {
        drivingToBlock = false;
        if (success) {
          stateMachine.SetNextState(new SpeedTapStateBeginMatch());
        }
        else {
          
        }
      });
    }
  }

  public override void Update() {
    base.Update();
    DriveToBlock();
  }
}


