using UnityEngine;
using System.Collections;

public class LookAtPatternConstruction : State {

  bool arrivedPose = false;
  float arrivedPoseTime = 0.0f;

  PatternPlayController patternPlayController = null;

  public override void Enter() {
    base.Enter();

    patternPlayController = (PatternPlayController)stateMachine.GetGameController();

    robot.GotoPose(0.0f, 0.0f, 0.0f, ArrivedPose, false, false);
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
        // check to see if blocks are in the right place

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
