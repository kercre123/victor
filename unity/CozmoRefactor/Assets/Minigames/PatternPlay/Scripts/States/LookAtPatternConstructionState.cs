using UnityEngine;
using System.Collections;

public class LookAtPatternConstructionState : State {

  bool arrivedPose = false;
  float arrivedPoseTime = 0.0f;

  PatternPlayGame patternPlayGame_ = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "LookAtPatternConstruction");

    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild = patternPlayGame_.GetAutoBuild();
    Vector3 idealViewPos = patternPlayAutoBuild.IdealViewPosition();
    robot.GotoPose(idealViewPos.x, idealViewPos.y, patternPlayAutoBuild.IdealViewAngle(), ArrivedPose, false, false);
  }

  public override void Update() {
    base.Update();
    if (arrivedPose) {
      if (Time.time - arrivedPoseTime < 2.0f) {
        if (patternPlayGame_.SeenPattern()) {
          stateMachine_.SetNextState(new CelebratePatternState());
        }
      }
      else if (patternPlayAutoBuild.BlocksInNeatList() == 3) {
        stateMachine_.SetNextState(new LookForPatternState());
      }
      else {
        stateMachine_.SetNextState(new LookForCubesState());
      }
    }
  }

  void ArrivedPose(bool success) {
    if (success) {
      arrivedPose = true;
      arrivedPoseTime = Time.time;
    }
    else {
      stateMachine_.SetNextState(new LookForCubesState());
    }
  }
}
