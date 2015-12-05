using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CubeLifting {

  public class FollowCubeUpDownState : State {

    private int _SelectedCubeId = -1;

    private float _CubeInvisibleTimer = 0f;
    private float _CubeOutOfBoundsTimer = 0f;

    private bool _StartedLifting = false;

    private Bounds _VisionBounds = new Bounds(new Vector3(150.0f, 0.0f, 100.0f ), new Vector3(50.0f, 50.0f, 200.0f));

    private bool _Up;
    private bool _RaiseLift;

    private float _LastHeight;

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
      _LastHeight = _Up ? 0 : 1;
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

      if (_CubeInvisibleTimer > 1.0f) {
        cube.SetLEDs(CozmoPalette.ColorToUInt(Color.white));
      }
      else if (_VisionBounds.Contains(_CurrentRobot.WorldToCozmo(cube.WorldPosition))) { 
        if (ValidateHeightChange(height)) {
          cube.SetLEDs(CozmoPalette.ColorToUInt(_Up ? Color.blue : Color.red));

          if (_RaiseLift) {
            // go to 80% so it doesn't block our view
            _CurrentRobot.SetLiftHeight(height * 0.8f);
          }

          _CurrentRobot.SetHeadAngle(height);
        }
        else {
          cube.SetLEDs(CozmoPalette.ColorToUInt(Color.yellow));
        }
        _CubeOutOfBoundsTimer = 0f;
        _StartedLifting = true;
      }
      else {
        cube.SetLEDs(CozmoPalette.ColorToUInt(Color.white));
        _CubeOutOfBoundsTimer += Time.deltaTime;
      }

      if ((_CubeInvisibleTimer > 10.0f || _CubeOutOfBoundsTimer > 10.0f) && _StartedLifting) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kShocked, HandleLoseAnimationDone);
        _StateMachine.SetNextState(animState);
        return; 
      }

      if (ReachedMoveEnd()) {

        if (_Settings == null || _Index + 1 >= _Settings.Count) {
          _CurrentRobot.LightCubes[_SelectedCubeId].TurnLEDsOff();
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kMajorWin, HandleWinAnimationDone);
          _StateMachine.SetNextState(animState);
        }
        else {
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kEnjoyLight, HandleStateCompleteAnimationDone);
          _StateMachine.SetNextState(animState);

        }
      }
    }

    private bool ValidateHeightChange(float height) {
      bool result = false;
      if (_Up) {
        result = height >= _LastHeight - 0.05f && height <= _LastHeight + 0.2f;
      }
      else {
        result = height <= _LastHeight + 0.05f && height >= _LastHeight - 0.2f;
      }
      if (result) {
        _LastHeight = height;
      }
      return result;
    }

    private bool ReachedMoveEnd() {
      return (_Up && _CurrentRobot.HeadAngle > CozmoUtil.kMaxHeadAngle * 0.9f * Mathf.Deg2Rad) || (!_Up && _CurrentRobot.HeadAngle < 0.05f);
    }

    private void HandleWinAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    private void HandleStateCompleteAnimationDone(bool success) {
      _StateMachine.SetNextState(new FollowCubeUpDownState(_Settings, _Index + 1, _SelectedCubeId));
    }

    private void HandleLoseAnimationDone(bool success) {
      _StateMachine.GetGame().RaiseMiniGameLose();
    }

    public override void Exit() {
      base.Exit();
    }
  }

}
