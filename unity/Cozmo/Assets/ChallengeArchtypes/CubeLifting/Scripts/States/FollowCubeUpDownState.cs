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

    private float _ProgressStart = 0f;
    private float _ProgressEnd = 1f;

    private ProceduralEyeParameters _LeftEyeParam = ProceduralEyeParameters.MakeDefaultLeftEye();
    private ProceduralEyeParameters _RightEyeParam = ProceduralEyeParameters.MakeDefaultRightEye();

    private CubeLiftingGame _GameInstance;

    public FollowCubeUpDownState(List<CubeLiftingSetting> settings, int index, int selectedCubeId = -1) {
      _Settings = settings;
      _Index = index;
      if (_Settings != null && index < _Settings.Count) {
        _Up = settings[index].Up;
        _RaiseLift = settings[index].RaiseLift;

        _ProgressStart = (_Index) / (float)_Settings.Count;
        _ProgressEnd = (_Index + 1) / (float)_Settings.Count;
      }
      _SelectedCubeId = selectedCubeId;
    }

    public override void Enter() {
      base.Enter();
      if (_SelectedCubeId == -1) {
        _SelectedCubeId = (_StateMachine.GetGame() as CubeLiftingGame).PickCube();
      }

      _CurrentRobot.TrackToObject(_CurrentRobot.LightCubes[_SelectedCubeId]);
      SetEyeParameters(_Up);
      _CurrentRobot.DisplayProceduralFace(0, Vector2.zero, Vector2.one, _LeftEyeParam, _RightEyeParam);

      _GameInstance = _StateMachine.GetGame() as CubeLiftingGame;
      _GameInstance.ShowHowToPlaySlide("CubeUpDown");
    }

    private void SetEyeParameters(bool up) {
      
      _LeftEyeParam.EyeCenter = new Vector2(_LeftEyeParam.EyeCenter.x, _LeftEyeParam.EyeCenter.y + (up ? -20f : 20f));
      _RightEyeParam.EyeCenter = new Vector2(_RightEyeParam.EyeCenter.x, _RightEyeParam.EyeCenter.y + (up ? -20f : 20f));
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

      _StateMachine.GetGame().Progress = Mathf.Lerp(_ProgressStart, _ProgressEnd, _Up ? height : 1 - height);

      if (_CubeInvisibleTimer > 0.5f) {
        cube.SetLEDs(CozmoPalette.ColorToUInt(Color.red));
      }
      else {
        cube.SetLEDs(CozmoPalette.ColorToUInt(Color.white));

        if (_RaiseLift) {
          // go to 80% so it doesn't block our view
          _CurrentRobot.SetLiftHeight(height * 0.8f);
        }

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
        AnimationState animState = new AnimationState();

        AnimationState.AnimationDoneHandler callback = null;
        if (_Settings == null || _Index + 1 >= _Settings.Count) {
          callback = HandleFinalStateCompleteAnimationDone;
        }
        else {
          callback = HandleStateCompleteAnimationDone;
        }

        animState.Initialize(AnimationName.kEnjoyLight, callback);
        _StateMachine.SetNextState(animState);
      }
    }

    private bool ReachedMoveEnd() {
      return (_Up && _CurrentRobot.HeadAngle > CozmoUtil.kMaxHeadAngle * 0.9f * Mathf.Deg2Rad) || (!_Up && _CurrentRobot.HeadAngle < 0.07f);
    }

    private void HandleStateCompleteAnimationDone(bool success) {
      _StateMachine.SetNextState(new FollowCubeUpDownState(_Settings, _Index + 1, _SelectedCubeId));
    }

    private void HandleFinalStateCompleteAnimationDone(bool success) {
      _GameInstance.ShowHowToPlaySlide("TapToLift");
      _CurrentRobot.TrackToObject(null);
      _StateMachine.SetNextState(new TapCubeState(new PickupCubeState(_SelectedCubeId), _SelectedCubeId));
    }

    private void HandleLoseAnimationDone(bool success) {
      (_StateMachine.GetGame() as CubeLiftingGame).LoseCubeSight();
    }

    private void HandleLifeLostAnimationDone(bool success) {
      // restart this state
      _StateMachine.SetNextState(new FollowCubeUpDownState(_Settings, _Index, _SelectedCubeId));
    }

    public override void Exit() {
      _CurrentRobot.StopTrackToObject();
      base.Exit();
    }
  }

}
