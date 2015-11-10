using UnityEngine;
using System.Collections;

public class LookAtPatternConstructionState : State {

  private bool _ArrivedPose = false;
  private float _ArrivedPoseTime = 0.0f;

  private PatternPlayGame _PatternPlayGame = null;
  private PatternPlayAutoBuild _PatternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "LookAtPatternConstruction");

    _PatternPlayGame = (PatternPlayGame)stateMachine_.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();
    Vector3 idealViewPos = _PatternPlayAutoBuild.IdealViewPosition();
    robot.GotoPose(idealViewPos.x, idealViewPos.y, _PatternPlayAutoBuild.IdealViewAngle(), ArrivedPose, false, false);
  }

  public override void Update() {
    base.Update();
    if (_ArrivedPose) {
      if (Time.time - _ArrivedPoseTime < 2.0f) {
        if (_PatternPlayGame.SeenPattern()) {
          stateMachine_.SetNextState(new CelebratePatternState());
        }
      }
      else if (_PatternPlayAutoBuild.BlocksInNeatList() == 3) {
        stateMachine_.SetNextState(new LookForPatternState());
      }
      else {
        stateMachine_.SetNextState(new LookForCubesState());
      }
    }
  }

  void ArrivedPose(bool success) {
    if (success) {
      _ArrivedPose = true;
      _ArrivedPoseTime = Time.time;
    }
    else {
      stateMachine_.SetNextState(new LookForCubesState());
    }
  }
}
