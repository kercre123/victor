using UnityEngine;
using System.Collections;

public class LookForGoldCubeState : State {

  bool _LookingAround = false;

  public override void Enter() {
    base.Enter();
  }

  public override void Update() {
    base.Update();
    for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
      if (_CurrentRobot.VisibleObjects[i] is LightCube) {
        _StateMachine.SetNextState(new FollowGoldCubeState());
      }
    }
  }

  void SearchForAvailableBlock() {
    if (_LookingAround == false) {
      _LookingAround = true;
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
    }
  }

  public override void Exit() {
    base.Exit();
    _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
    _LookingAround = false;
  }

}
