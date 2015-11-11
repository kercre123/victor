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

    _PatternPlayGame = (PatternPlayGame)_StateMachine.GetGame();
    _PatternPlayAutoBuild = _PatternPlayGame.GetAutoBuild();
    Vector3 idealViewPos = _PatternPlayAutoBuild.IdealViewPosition();
    _CurrentRobot.GotoPose(idealViewPos.x, idealViewPos.y, _PatternPlayAutoBuild.IdealViewAngle(), ArrivedPose, false, false);
  }

  public override void Update() {
    base.Update();
    if (_ArrivedPose) {
      if (Time.time - _ArrivedPoseTime < 2.0f) {
        if (_PatternPlayGame.SeenPattern()) {
          _StateMachine.SetNextState(new CelebratePatternState());
        }
      }
      else if (_PatternPlayAutoBuild.BlocksInNeatList() == 3) {
        _StateMachine.SetNextState(new LookForPatternState());
      }
      else {
        _StateMachine.SetNextState(new LookForCubesState());
      }
    }
  }

  void ArrivedPose(bool success) {
    if (success) {
      _ArrivedPose = true;
      _ArrivedPoseTime = Time.time;
    }
    else {
      _StateMachine.SetNextState(new LookForCubesState());
    }
  }
}
