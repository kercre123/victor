using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace ForwardBackward {

  public class FollowCubeForwardBackwardState : State {

    private Vector3 _StartPosition;
    private int _SelectedCubeId = -1;

    // don't use the rect constructor position because
    // it is top left and not center of the rect.
    private Bounds _VisionBounds = new Bounds();

    private bool _Forward;
    private float _DistanceRequired;

    private List<ForwardBackwardSetting> _Settings;
    private int _Index;

    public FollowCubeForwardBackwardState(List<ForwardBackwardSetting> settings, int index, int selectedCubeId = -1) {
      _Settings = settings;
      _Index = index;
      if (_Settings != null && index < _Settings.Count) {
        _Forward = settings[index].Forward;
        _DistanceRequired = settings[index].Distance;
      }
      _SelectedCubeId = selectedCubeId;
    }
      
    public override void Enter() {
      base.Enter();
      if (_SelectedCubeId == -1) {
        _SelectedCubeId = (_StateMachine.GetGame() as ForwardBackwardGame).PickCube();
      }
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(0.0f);

      _VisionBounds.size = new Vector3(100.0f, 50.0f, 50.0f);
      _VisionBounds.center = new Vector3(150.0f+ (_Forward ? 50f : -50f), 0.0f, 25.0f );
      _StartPosition = _CurrentRobot.WorldPosition;
    }

    public override void Update() {
      base.Update();

      bool cubeInBoundsNow = false;

      for (int i = 0; i < _CurrentRobot.VisibleObjects.Count; ++i) {
        if (_CurrentRobot.VisibleObjects[i].ID == _SelectedCubeId) {
          
          Vector3 cubePositionCozmoSpace = (Vector3)_CurrentRobot.WorldToCozmo(_CurrentRobot.VisibleObjects[i].WorldPosition);

          Debug.Log(cubePositionCozmoSpace);
          if (_VisionBounds.Contains(cubePositionCozmoSpace)) {
            cubeInBoundsNow = true;
            break;
          }
        }
      }

      if (cubeInBoundsNow) {        
        _CurrentRobot.LightCubes[_SelectedCubeId].SetLEDs(CozmoPalette.ColorToUInt(_Forward ? Color.blue : Color.red));

        var speed = _Forward ? 30f : -30f;
        _CurrentRobot.DriveWheels(speed, speed);
      }
      else {
        _CurrentRobot.LightCubes[_SelectedCubeId].SetLEDs(CozmoPalette.ColorToUInt(Color.white));

        _CurrentRobot.DriveWheels(0, 0);
      }

      if ((_StartPosition - _CurrentRobot.WorldPosition).sqrMagnitude >= _DistanceRequired * _DistanceRequired) {

        if (_Settings == null || _Index + 1 >= _Settings.Count) {
          _CurrentRobot.LightCubes[_SelectedCubeId].SetLEDs(CozmoPalette.ColorToUInt(Color.black));
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kMajorWin, HandleWinAnimationDoneHandler);
          _StateMachine.SetNextState(animState);
        }
        else {
          AnimationState animState = new AnimationState();
          animState.Initialize(AnimationName.kEnjoyLight, HandleStateCompleteAnimationDoneHandler);
          _StateMachine.SetNextState(animState);

        }
      }
    }

    private void HandleWinAnimationDoneHandler(bool success) {
      _StateMachine.GetGame().RaiseMiniGameWin();
    }

    private void HandleStateCompleteAnimationDoneHandler(bool success) {
      _StateMachine.SetNextState(new FollowCubeForwardBackwardState(_Settings, _Index + 1, _SelectedCubeId));
    }

    public override void Exit() {
      base.Exit();
    }
  }

}
