using System;

namespace SpeedTap {

  public class SpeedTapStateGoToCube : State {

    private SpeedTapGame _SpeedTapGame = null;
    private bool _DrivingToBlock = false;

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _CurrentRobot.SetLiftHeight(1.0f);
      _CurrentRobot.SetHeadAngle();
    }

    void DriveToBlock() {
      if (_DrivingToBlock)
        return;
      if (_SpeedTapGame.CozmoBlock.MarkersVisible) {
        _DrivingToBlock = true;
        _CurrentRobot.AlignWithObject(_SpeedTapGame.CozmoBlock, 90.0f, (bool success) => {
          _DrivingToBlock = false;
          if (success) {
            _StateMachine.SetNextState(new SpeedTapStateBeginMatch());
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

}
