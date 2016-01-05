using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // Chase after the current target, changes to confused state if 
  // no cubes are active.
  // Changes to Panic state if Both cubes are active.
  // Plays Celebration and deactivates cube before returning to Idle state if
  // Cozmo successfully arrives at target.
  // If Cozmo loses track of their target, enter confused state.
  public class WhackAMoleChase : State {
    private WhackAMoleGame _WhackAMoleGame;
    private LightCube lastChasedCube;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      lastChasedCube = _WhackAMoleGame.CurrentTarget;
      _CurrentRobot.GotoObject(_WhackAMoleGame.CurrentTarget, 5.0f, RobotArrives);
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }

    public void RobotArrives(bool success) {
      LightCube cube;
      if (success) {
        if (_CurrentRobot.LightCubes.TryGetValue(_WhackAMoleGame.CurrentTarget.ID, out cube)) {
          _WhackAMoleGame.ToggleCube(cube.ID);
          _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, HandleAnimationDone));
          return;
        }
      }
      _StateMachine.SetNextState(new WhackAMoleConfusion());
    }

    void HandleAnimationDone(bool success) {
      if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.NONE) {
        // Return to Idle if all cubes are off.
        _StateMachine.SetNextState(new WhackAMoleIdle());
      }
      else {
        // A cube has been tapped, start chase. If more than one cube is
        // active, Chase will handle moving to Panic.
        _StateMachine.SetNextState(new WhackAMoleChase());
      }
    }

    public override void Update() {
      base.Update();
    }

    void HandleMoleStateChange(WhackAMoleGame.MoleState state) {
      if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.NONE) {
        // A cube has been tapped, start chase. If more than one cube is
        // active, Chase will handle moving to Panic.
        _StateMachine.SetNextState(new WhackAMoleConfusion());
      }
      else if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.BOTH) {
        _StateMachine.SetNextState(new WhackAMolePanic());
      }
      else if (lastChasedCube.ID != _WhackAMoleGame.CurrentTarget.ID) {
        lastChasedCube = _WhackAMoleGame.CurrentTarget;
        _CurrentRobot.GotoObject(_WhackAMoleGame.CurrentTarget, 5.0f, RobotArrives);
      }
    }

    public override void Exit() {
      base.Exit();
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
