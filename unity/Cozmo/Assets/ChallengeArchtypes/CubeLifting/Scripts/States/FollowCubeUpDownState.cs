using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CubeLifting {

  public class FollowCubeUpDownState : State {

    private int _SelectedCubeId = -1;

    private float _CubeInvisibleTimer = 0f;

    private bool _StartedLifting = false;

    private bool _Up;
    private bool _RaiseLift;

    private List<CubeLiftingSetting> _Settings;
    private int _Index;

    public FollowCubeUpDownState(List<CubeLiftingSetting> settings, int index, int selectedCubeId = -1) {
      _Settings = settings;
      _Index = index;
      if (_Settings != null && index < _Settings.Count) {
        _Up = settings[index].Up;
        _RaiseLift = settings[index].RaiseLift;
      }
      _SelectedCubeId = selectedCubeId;
    }
      
    public override void Enter() {
      base.Enter();
      if (_SelectedCubeId == -1) {
        _SelectedCubeId = (_StateMachine.GetGame() as CubeLiftingGame).PickCube();
      }
    }

    public override void Update() {
      base.Update();

      bool cubeVisible = false;

      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        if (_CurrentRobot.VisibleObjects[i].ID == _SelectedCubeId) {
          cubeVisible = true;
          break;
        }
      }

      if (cubeVisible) {
        _CubeInvisibleTimer = 0f;
      }
      else {
        _CubeInvisibleTimer += Time.deltaTime;
      }
        
      var cube = _CurrentRobot.LightCubes[_SelectedCubeId];

      float height = Mathf.InverseLerp(30, 150, cube.WorldPosition.z);

      if (_CubeInvisibleTimer > 0.5f) {
        cube.SetLEDs(CozmoPalette.ColorToUInt(Color.white));
      }
      else {
        cube.SetLEDs(CozmoPalette.ColorToUInt(_Up ? Color.blue : Color.red));

        if (_RaiseLift) {
          // go to 80% so it doesn't block our view
          _CurrentRobot.SetLiftHeight(height * 0.8f);
        }

        _CurrentRobot.SetHeadAngle(height);

        _StartedLifting = true;
      }
        
      if (_CubeInvisibleTimer > 5.0f && _StartedLifting) {

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
        return; 
      }

      if (ReachedMoveEnd()) {

        if (_Settings == null || _Index + 1 >= _Settings.Count) {
          _StateMachine.SetNextState(new PickupCubeState(_SelectedCubeId));
        }
        else {
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kEnjoyLight, HandleStateCompleteAnimationDone);
          _StateMachine.SetNextState(animState);

        }
      }
    }

    private bool ReachedMoveEnd() {
      return (_Up && _CurrentRobot.HeadAngle > CozmoUtil.kMaxHeadAngle * 0.9f * Mathf.Deg2Rad) || (!_Up && _CurrentRobot.HeadAngle < 0.07f);
    }

    private void HandleStateCompleteAnimationDone(bool success) {
      _StateMachine.SetNextState(new FollowCubeUpDownState(_Settings, _Index + 1, _SelectedCubeId));
    }

    private void HandleLoseAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameLose();
    }

    private void HandleLifeLostAnimationDone(bool success) {
      // restart this state
      _StateMachine.SetNextState(new FollowCubeUpDownState(_Settings, _Index, _SelectedCubeId));
    }

    public override void Exit() {
      base.Exit();
    }
  }

}
