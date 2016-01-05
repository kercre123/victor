using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // TODO: Chase after the current target, changes to confused state if 
  // no cubes are active.
  // Changes to Panic state if Both cubes are active.
  // Plays Celebration and deactivates cube before returning to Idle state if
  // Cozmo successfully arrives at target.
  // If Cozmo loses track of their target, reset and return to Idle.
  public class WhackAMoleChase : State {
    private WhackAMoleGame _WhackAMoleGame;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      if (_WhackAMoleGame.CurrentTarget == null) {
        _WhackAMoleGame.SetUpCubes();
      }
      else {
        _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
        _CurrentRobot.GotoObject(_WhackAMoleGame.CurrentTarget, 5.0f, RobotArrives);
      }
    }

    public void RobotArrives(bool success) {
      LightCube cube;
      if (success) {
        if (_CurrentRobot.LightCubes.TryGetValue(_WhackAMoleGame.CurrentTarget.ID, out cube)) {
          
        }
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
