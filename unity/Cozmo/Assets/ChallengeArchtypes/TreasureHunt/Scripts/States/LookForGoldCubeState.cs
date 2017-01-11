using UnityEngine;
using System.Collections;

namespace TreasureHunt {

  public class LookForGoldCubeState : State {

    bool _LookingAround = false;

    public override void Enter() {
      base.Enter();
      (_StateMachine.GetGame() as TreasureHuntGame).PickNewGoldPosition();
    }

    public override void Update() {
      base.Update();
      for (int i = 0; i < _CurrentRobot.VisibleLightCubes.Count; ++i) {
        _StateMachine.SetNextState(new FollowGoldCubeState());
      }
    }

    void SearchForAvailableBlock() {
      if (_LookingAround == false) {
        _LookingAround = true;
        _CurrentRobot.ExecuteBehaviorByExecutableType(Anki.Cozmo.ExecutableBehaviorType.LookAroundInPlace);
      }
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.ExecuteBehaviorByExecutableType(Anki.Cozmo.ExecutableBehaviorType.NoneBehavior);
      }
      _LookingAround = false;
    }

  }

}
