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
    private const float kPounceDist = 60f;
    private const float kTargetDist = 80f;
    private WhackAMoleGame _WhackAMoleGame;
    private KeyValuePair<int,int> _TargetKvP;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _TargetKvP = _WhackAMoleGame.CurrentTargetKvP;
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key], kTargetDist, AlignDone, true, 
        _WhackAMoleGame.GetRelativeRad(_TargetKvP), Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING);
      _WhackAMoleGame.FixCozmoAngles();
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }

    public void AlignDone(bool success) {
      if (_WhackAMoleGame.CubeState != WhackAMoleGame.MoleState.SINGLE) {
        return;
      }
      if (_WhackAMoleGame.ActivatedFaces.Contains(_TargetKvP)) {
        _CurrentRobot.GotoPose(_WhackAMoleGame.GetRelativePos(_TargetKvP),
          _CurrentRobot.Rotation, false, false, RobotArrives, Anki.Cozmo.QueueActionPosition.NOW);
      }
      else {
        _StateMachine.SetNextState(new WhackAMoleConfusion());
      }

    }

    public void RobotArrives(bool success) {
      if (_WhackAMoleGame.ActivatedFaces.Contains(_TargetKvP)) {
        float dist = Vector3.Distance(_CurrentRobot.LightCubes[_TargetKvP.Key].WorldPosition, _CurrentRobot.WorldPosition);
        DAS.Debug(this, string.Format("DistCheck -- {0}", dist));
        if (dist < kPounceDist) {
          _StateMachine.SetNextState(new AnimationState(AnimationName.kSpeedTap_Tap_01, HandleAnimationDone));
        }
        else {
          // TODO: Potentially figure out other way to handle failure in this case
          _CurrentRobot.GotoPose(_WhackAMoleGame.GetRelativePos(_TargetKvP),
            _CurrentRobot.Rotation, false, false, RobotArrives, Anki.Cozmo.QueueActionPosition.NOW);
        }
      }
      else {
        _StateMachine.SetNextState(new WhackAMoleConfusion());
      }
    }

    void HandleAnimationDone(bool success) {
      _WhackAMoleGame.ToggleCubeFace(_TargetKvP);
      _StateMachine.SetNextState(new WhackAMoleIdle());
    }

    public override void Update() {
      base.Update();
      if (_CurrentRobot.IsBusy == false) {
        _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key], kTargetDist, AlignDone, true, 
          _WhackAMoleGame.GetRelativeRad(_TargetKvP), Anki.Cozmo.QueueActionPosition.NEXT);
        _WhackAMoleGame.FixCozmoAngles();
      }
    }

    void HandleMoleStateChange(WhackAMoleGame.MoleState state) {
      DAS.Debug(this, string.Format("Chase - Mole State Changed to {0}", state));
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
        _CurrentRobot.CancelCallback(AlignDone);
        _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[_TargetKvP.Key], kTargetDist, AlignDone, true, 
          _WhackAMoleGame.GetRelativeRad(_TargetKvP), Anki.Cozmo.QueueActionPosition.NEXT);
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(AlignDone);
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
