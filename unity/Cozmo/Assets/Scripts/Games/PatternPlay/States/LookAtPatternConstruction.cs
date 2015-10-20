using UnityEngine;
using System.Collections;

public class LookAtPatternConstruction : State {

  bool arrivedPose = false;
  float arrivedPoseTime = 0.0f;

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();

    DAS.Info("PatternPlayState", "LookAtPatternConstruction");

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
