using UnityEngine;
using System.Collections;

public class LookForGoldCubeState : State {

  bool _LookingAround = false;

  public override void Enter() {
    base.Enter();
  }

  public override void Update() {
    base.Update();
    for (int i = 0; i < robot.VisibleObjects.Count; ++i) {
      if (robot.VisibleObjects[i] is LightCube) {
        stateMachine_.SetNextState(new FollowGoldCubeState());
      }
    }
  }

  void SearchForAvailableBlock() {
    if (_LookingAround == false) {
      _LookingAround = true;
      robot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
    }
  }

  public override void Exit() {
    base.Exit();
    robot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    _LookingAround = false;
  }

}
