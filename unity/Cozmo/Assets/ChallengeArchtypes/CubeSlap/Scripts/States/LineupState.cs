using UnityEngine;
using System.Collections;

namespace CubeSlap {
  // Line up with the Cube and get into position, then transfer into the SlapGameState once lined up
  public class LineupState : State {

    private CubeSlapGame _CubeSlapGame;
    private int _SelectedCubeId = -1;

    public override void Enter() {
      base.Enter();
      _CubeSlapGame = (_StateMachine.GetGame() as CubeSlapGame);

      LightCube currentTarget = _CubeSlapGame.GetCurrentTarget();
      if (currentTarget != null) {
        currentTarget.SetLEDs(Color.white);
      }
      else {
        // If we don't have a target for some reason, return to the seek state and keep waiting
        _StateMachine.SetNextState(new SeekState());
        return;
      }

      if (_SelectedCubeId == -1) {
        _SelectedCubeId = currentTarget.ID;
      }
      _CurrentRobot.SetHeadAngle(0.25f);
      _CurrentRobot.SetLiftHeight(1.0f);
      // Using the identified cube, move to pick up the cube.
      _CurrentRobot.GotoObject(_CurrentRobot.LightCubes[_SelectedCubeId], 5.0f, callback: OnCubeLineUp);
    }

    // Cube Pickup attempt
    // On failure, back up to original position and return to SeekState
    // On Success, place the Cube on the ground in front of you, place lift on top of the cube,
    // Once that motion is complete, do one final check (with the same failure condition of resetting)
    // then advance to the SlapGameState if successful.
    private void OnCubeLineUp(bool success) {
      if (success) {
        // Attempt to mount cube.
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kTapCube, HandleAnimConfirmState);
        _StateMachine.SetNextState(animState);
      }
      else {
        // Reset to the SeekState, something went wrong.
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kByeBye, HandleAnimFailState);
        _StateMachine.SetNextState(animState);
      }
    }

    public void HandleAnimFailState(bool success) {
      _StateMachine.SetNextState(new SeekState());
    }

    public void HandleAnimConfirmState(bool success) {
      if (success) {
        _StateMachine.SetNextState(new SlapGameState());
      }
      else {
        _StateMachine.SetNextState(new SeekState());
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

