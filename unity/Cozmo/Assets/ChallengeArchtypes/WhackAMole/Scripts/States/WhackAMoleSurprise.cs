using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WhackAMole {
  // Chase after the current target, changes to confused state if 
  // no cubes are active.
  // Changes to Panic state if Both cubes are active.
  // Plays Celebration and deactivates cube before returning to Idle state if
  // Cozmo successfully arrives at target.
  // If Cozmo loses track of their target, enter confused state.
  public class WhackAMoleSurprise: State {
    private WhackAMoleGame _WhackAMoleGame;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      KeyValuePair<int,int> _TargetKvP = _WhackAMoleGame.CurrentTargetKvP;
      _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key],0f ,WentToObject ,true , 
        _WhackAMoleGame.GetRelativeRad(_TargetKvP) , Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING);
    }

    void WentToObject(bool success) {
      _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleAnimationDone));
    }

    void HandleAnimationDone(bool success) {
      if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.BOTH) {
        _StateMachine.SetNextState(new WhackAMolePanic());
      }
      else if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.SINGLE) {
        _StateMachine.SetNextState(new WhackAMoleChase());
      }
      else {
        _StateMachine.SetNextState(new WhackAMoleConfusion());
      }
    }

    public override void Update() {
      base.Update();
    }

    public override void Exit() {
      base.Exit();
    }
  }
}
