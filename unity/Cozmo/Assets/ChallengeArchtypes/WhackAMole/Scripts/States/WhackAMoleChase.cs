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
    private LightCube _LastCubeTarget;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      _LastCubeTarget = _WhackAMoleGame.CurrentTarget;
      _CurrentRobot.PickupObject(_WhackAMoleGame.CurrentTarget,true,false,false,0,RobotArrives);
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }

    public void RobotArrives(bool success) {
      LightCube cube;
      if (success) {
        if (_WhackAMoleGame.ActivatedCubes.TryGetValue(_WhackAMoleGame.CurrentTarget.ID, out cube)) {
          _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
          _WhackAMoleGame.ToggleCube(cube.ID);
          _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, HandleAnimationDone));
        }
      }
      else {
        if (_WhackAMoleGame.CurrentTarget != null) {
          _CurrentRobot.PickupObject(_WhackAMoleGame.CurrentTarget,true,false,false,0,RobotArrives);
        }
      }
    }

    void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new WhackAMoleIdle());
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
      else if (_LastCubeTarget.ID != _WhackAMoleGame.CurrentTarget.ID) {
        _LastCubeTarget = _WhackAMoleGame.CurrentTarget;
        _CurrentRobot.PickupObject(_WhackAMoleGame.CurrentTarget,true,false,false,0,RobotArrives);
      }
    }

    public override void Exit() {
      base.Exit();
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
