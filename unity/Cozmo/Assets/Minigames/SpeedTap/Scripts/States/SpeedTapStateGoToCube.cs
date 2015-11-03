using System;


public class SpeedTapStateGoToCube : State {

  private SpeedTapGame speedTapGame_ = null;
  private bool drivingToBlock_ = false;

  public override void Enter() {
    base.Enter();
    speedTapGame_ = stateMachine_.GetGame() as SpeedTapGame;
    robot.SetLiftHeight(1.0f);
    robot.SetHeadAngle();
  }

  void DriveToBlock() {
    if (drivingToBlock_)
      return;
    if (speedTapGame_.cozmoBlock_.MarkersVisible) {
      drivingToBlock_ = true;
      robot.GotoObject(speedTapGame_.cozmoBlock_, 50.0f, (bool success) => {
        drivingToBlock_ = false;
        if (success) {
          stateMachine_.SetNextState(new SpeedTapStateBeginMatch());
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


