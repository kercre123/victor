using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CubeLifting {

  public class PickupCubeState : State {

    private int _SelectedCubeId = -1;

    public PickupCubeState(int selectedCubeId = -1) {
      _SelectedCubeId = selectedCubeId;
    }

    public override void Enter() {
      base.Enter();
      if (_SelectedCubeId == -1) {
        _SelectedCubeId = ((CubeLiftingGame)_StateMachine.GetGame()).PickCube();
      }

      _CurrentRobot.PickupObject(_CurrentRobot.LightCubes[_SelectedCubeId], callback: OnCubePickedUp);
    }

    private void OnCubePickedUp(bool success) {
      if (success) {
        var game = _StateMachine.GetGame();
        if (game.TryDecrementAttempts()) {
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kSeeOldPattern, HandleLifeLostAnimationDone);
          _StateMachine.SetNextState(animState);
        }
        else {          
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kShocked, HandleLoseAnimationDone);
          _StateMachine.SetNextState(animState);
        }
      }
      else {
        _CurrentRobot.LightCubes[_SelectedCubeId].TurnLEDsOff();
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kMajorWin, HandleWinAnimationDone);
        _StateMachine.SetNextState(animState);
      }
    }

    private void HandleWinAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    private void HandleLoseAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameLose();
    }

    private void HandleLifeLostAnimationDone(bool success) {
      // restart this state
      _StateMachine.SetNextState(new PickupCubeState(_SelectedCubeId));
    }

    public override void Exit() {
      base.Exit();
    }
  }

}
