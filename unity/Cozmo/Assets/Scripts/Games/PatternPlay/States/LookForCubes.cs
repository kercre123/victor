using UnityEngine;
using System.Collections;

public class LookForCubes : State {

  PatternPlayController patternPlayController = null;
  PatternPlayAutoBuild patternPlayAutoBuild = null;

  public override void Enter() {
    base.Enter();
    patternPlayController = (PatternPlayController)stateMachine.GetGameController();
    patternPlayAutoBuild = patternPlayController.GetAutoBuild();
  }

  public override void Update() {
    base.Update();
    if (patternPlayAutoBuild.AvailableBlocks() > 0) {
      stateMachine.SetNextState(new PickUpCube());
    }
    else {
      SearchForAvailableBlock();
    }
  }

  void SearchForAvailableBlock() {
    
  }
}
