using UnityEngine;
using System.Collections;

public class CelebrateGoldState : State {

  public override void Enter() {
    base.Enter();
    robot.SendAnimation("majorWin", AnimationDone);
  }

  void AnimationDone(bool success) {
    // play animation on phone screen
    (stateMachine_.GetGame() as TreasureHuntGame).ClearBlockLights();
  }

  // called when next hunt button is pressed.
  void OnNextHunt() {
    (stateMachine_.GetGame() as TreasureHuntGame).PickNewGoldPosition();
    stateMachine_.SetNextState(new LookForGoldCubeState());
  }

  public override void Exit() {
    base.Exit();
  }
}
