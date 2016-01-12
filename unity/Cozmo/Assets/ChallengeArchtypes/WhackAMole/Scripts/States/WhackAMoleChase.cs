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
    private KeyValuePair<int,int> _TargetKvP;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      _TargetKvP = _WhackAMoleGame.CurrentTargetKvP;
      _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key], 25f , RobotArrives,true, 
        _WhackAMoleGame.GetRelativeRad(_TargetKvP), Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING);

      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }

    public void RobotArrives(bool success) {
      if (_WhackAMoleGame.CubeState != WhackAMoleGame.MoleState.SINGLE) {
        return;
      }
      if (success) {
        if (_WhackAMoleGame.ActivatedFaces.Contains(_TargetKvP)) {
          _StateMachine.SetNextState(new AnimationState(AnimationName.kTapCube, HandleAnimationDone));
        }
      }
      else {

        if ((_TargetKvP.Equals(new KeyValuePair<int, int>(-1,-1))) || !_WhackAMoleGame.ActivatedFaces.Contains(_TargetKvP)) {
          _StateMachine.SetNextState(new WhackAMoleConfusion());
        }else {
          _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key], 25f , RobotArrives,true, 
            _WhackAMoleGame.GetRelativeRad(_TargetKvP), Anki.Cozmo.QueueActionPosition.NEXT);
        }
      }
    }

    void HandleAnimationDone(bool success) {
      _WhackAMoleGame.ToggleCubeFace(_TargetKvP);
      _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, ReturnToIdle));
    }

    void ReturnToIdle(bool success) {
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
        _StateMachine.SetNextState(new WhackAMoleSurprise());
      }
      else if (!_TargetKvP.Equals(_WhackAMoleGame.CurrentTargetKvP)) {
        // A different cube is the target now.
        _TargetKvP = _WhackAMoleGame.CurrentTargetKvP;
        _CurrentRobot.CancelCallback(RobotArrives);
        _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key], 25f , RobotArrives,true, 
          _WhackAMoleGame.GetRelativeRad(_TargetKvP), Anki.Cozmo.QueueActionPosition.NEXT);
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(RobotArrives);
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
