using System;


public class SpeedTapStateGoToCube : State {

  private SpeedTapGame _SpeedTapGame = null;
  private bool _DrivingToBlock = false;

  public override void Enter() {
    base.Enter();
    _SpeedTapGame = stateMachine_.GetGame() as SpeedTapGame;
    robot.SetLiftHeight(1.0f);
    robot.SetHeadAngle();
  }

  void DriveToBlock() {
    if (_DrivingToBlock)
      return;
    if (_SpeedTapGame.CozmoBlock.MarkersVisible) {
      _DrivingToBlock = true;
      robot.AlignWithObject(_SpeedTapGame.CozmoBlock, 90.0f, (bool success) => {
        _DrivingToBlock = false;
        if (success) {
          stateMachine_.SetNextState(new SpeedTapStateBeginMatch());
        }
        else {
          DAS.Debug("SpeedTapStateGoToCube", "AlignWithObject Failed");
        }
      });
    }
  }

  public override void Update() {
    base.Update();
    DriveToBlock();
  }
}


