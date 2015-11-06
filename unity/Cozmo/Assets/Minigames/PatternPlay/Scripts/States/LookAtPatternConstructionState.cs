using UnityEngine;
using System.Collections;

public class LookAtPatternConstructionState : State {

  private bool arrivedPose = false;
  private float arrivedPoseTime = 0.0f;

  private PatternPlayGame patternPlayGame_ = null;
  private PatternPlayAutoBuild patternPlayAutoBuild_ = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "LookAtPatternConstruction");

    patternPlayGame_ = (PatternPlayGame)stateMachine_.GetGame();
    patternPlayAutoBuild_ = patternPlayGame_.GetAutoBuild();
    Vector3 idealViewPos = patternPlayAutoBuild_.IdealViewPosition();
    robot.GotoPose(idealViewPos.x, idealViewPos.y, patternPlayAutoBuild_.IdealViewAngle(), ArrivedPose, false, false);
  }

  public override void Update() {
    base.Update();
    if (arrivedPose) {
      if (Time.time - arrivedPoseTime < 2.0f) {
        if (patternPlayGame_.SeenPattern()) {
          stateMachine_.SetNextState(new CelebratePatternState());
        }
      }
      else if (patternPlayAutoBuild_.BlocksInNeatList() == 3) {
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
