using UnityEngine;
using System.Collections;

public class LookAtPatternConstruction : State {

  bool arrivedPose = false;
  float arrivedPoseTime = 0.0f;

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
    Vector3 idealViewPos = patternPlayAutoBuild.IdealViewPosition();
    robot.GotoPose(idealViewPos.x, idealViewPos.y, patternPlayAutoBuild.IdealViewAngle(), ArrivedPose, false, false);
  }

  public override void Update() {
    base.Update();
    if (arrivedPose) {
      if (Time.time - arrivedPoseTime < 2.0f) {
        if (patternPlayController.SeenPattern()) {
          stateMachine.SetNextState(new CelebratePattern());
        }
      }
      else {
        // this is not the right pattern... reset the neat list
        patternPlayAutoBuild.ClearNeatList();
        stateMachine.SetNextState(new LookForCubes());
      }
    }
  }

  void ArrivedPose(bool success) {
    if (success) {
      arrivedPose = true;
      arrivedPoseTime = Time.time;
    }
    else {
      stateMachine.SetNextState(new LookForCubes());
    }
  }
}
