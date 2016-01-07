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
  public class WhackAMoleChase : State {
    private WhackAMoleGame _WhackAMoleGame;
    private LightCube _LastCubeTarget;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      _LastCubeTarget = _WhackAMoleGame.CurrentTarget;
      Debug.Log(string.Format("Chase - Start Targeting Cube {0}", _LastCubeTarget.ID));
      _CurrentRobot.GotoObject(_WhackAMoleGame.CurrentTarget,85f,RobotArrives);
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }

    public void RobotArrives(bool success) {
      LightCube cube;
      if (_WhackAMoleGame.CubeState != WhackAMoleGame.MoleState.SINGLE) {
        return;
      }
      if (success) {
        if (_WhackAMoleGame.ActivatedCubes.TryGetValue(_WhackAMoleGame.CurrentTarget.ID, out cube)) {
          _WhackAMoleGame.ToggleCube(cube.ID);
          _StateMachine.SetNextState(new AnimationState(AnimationName.kHappyA, HandleAnimationDone));
        }
      }
      else {

        if (_WhackAMoleGame.CurrentTarget == null) {
          _StateMachine.SetNextState(new WhackAMoleConfusion());
          
        }else {
          // Attempt to offset self
          _CurrentRobot.GotoObject(_WhackAMoleGame.CurrentTarget, 85f, RobotArrives);
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
      Debug.Log(string.Format("Chase - Mole State Changed to {0}", state));
      if (state == WhackAMoleGame.MoleState.NONE) {
        // A cube has been tapped, start chase. If more than one cube is
        // active, Chase will handle moving to Panic.
        _StateMachine.SetNextState(new WhackAMoleConfusion());
      }
      else if (state == WhackAMoleGame.MoleState.BOTH) {
        _StateMachine.SetNextState(new WhackAMolePanic());
      }
      else if (_LastCubeTarget.ID != _WhackAMoleGame.CurrentTarget.ID) {
        // A different cube is the target now.
        _LastCubeTarget = _WhackAMoleGame.CurrentTarget;
        //_CurrentRobot.PickupObject(_WhackAMoleGame.CurrentTarget,true,false,false,0,RobotArrives);
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(RobotArrives);
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
