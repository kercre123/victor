using UnityEngine;
using System.Collections;

namespace TreasureHunt {

  public class CelebrateGoldState : State {

    public override void Enter() {
      base.Enter();
      _CurrentRobot.SendAnimation(AnimationName.kMajorWin, HandleAnimationDone);
    }

    void HandleAnimationDone(bool success) {
      // play animation on phone screen
      (_StateMachine.GetGame() as TreasureHuntGame).ClearBlockLights();
      OnNextHunt();
    }

    // called when next hunt button is pressed.
    void OnNextHunt() {
      (_StateMachine.GetGame() as TreasureHuntGame).PickNewGoldPosition();
      _StateMachine.SetNextState(new LookForGoldCubeState());
    }

    public override void Exit() {
      base.Exit();
    }
  }

}
