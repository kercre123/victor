using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace RotationTraining {
  public class RotateState : State {

    public enum RotateCubeState {
      LEFT,
      RIGHT
    }

    private int _CubeID = -1;
    private Color[] _RotateCubeColors = { Color.blue, Color.green };
    private RotateCubeState _RotateCubeState = RotateCubeState.LEFT;
    private float _LastRotateCubeStateChanged;

    private float _CurrentWheelLeftSpeed = 0.0f;
    private float _CurrentWheelRightSpeed = 0.0f;

    private float _CubeShakeWheelSpeedMmpsModifier = 300f;

    // Value from 0 to 1
    private float _WheelSpeedDecayModifier = 0.75f;

    public override void Enter() {
      base.Enter();
      _LastRotateCubeStateChanged = Time.time;
      LightCube.MovedAction += OnCubeMoved;

      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      // PoseAngle gets reset to 0 automatically on state enter.
      // Range is -PI to PI
      // _CurrentRobot.PoseAngle = 0;

      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        _CubeID = kvp.Key;
        break;
      }

      // TODO: Start timer for lose state
    }

    public override void Update() {

      base.Update();
      // TODO: Use better change method
      if (_LastRotateCubeStateChanged > 2.0f && Random.Range(0.0f, 1.0f) > 0.98f) {
        ChangeRotateCubeState();
      }

      // celebrate when he is at 180... TODO: don't let players cheat by rotating cozmo manually?
      float marginOfError = 0.9f;
      if (_CurrentRobot.PoseAngle > Mathf.PI * marginOfError || _CurrentRobot.PoseAngle < -Mathf.PI * marginOfError) {
        _StateMachine.SetNextState(new CelebrateState());
      }

      // set light cube color based on _RotateCubeState
      _CurrentRobot.LightCubes[_CubeID].SetLEDs(CozmoPalette.ColorToUInt(_RotateCubeColors[(int)_RotateCubeState]));

      // Decay the wheel speed over time if the cube doesn't move
      _CurrentWheelLeftSpeed = Mathf.Lerp(_CurrentWheelLeftSpeed, 0.0f, _WheelSpeedDecayModifier * Time.deltaTime);
      _CurrentWheelRightSpeed = Mathf.Lerp(_CurrentWheelRightSpeed, 0.0f, _WheelSpeedDecayModifier * Time.deltaTime);

      _CurrentRobot.DriveWheels(_CurrentWheelLeftSpeed * Time.deltaTime, _CurrentWheelRightSpeed * Time.deltaTime);
    }

    public override void Exit() {
      base.Exit();
    }

    private void OnCubeMoved(int cubeID, float x, float y, float z) {

      if (cubeID != _CubeID) {
        return;
      }

      if (_RotateCubeState == RotateCubeState.LEFT) {
        if (_CurrentWheelRightSpeed < 0 || _CurrentWheelLeftSpeed > 0) {
          _CurrentWheelRightSpeed = 0;
          _CurrentWheelLeftSpeed = 0;
        }
        _CurrentWheelRightSpeed += _CubeShakeWheelSpeedMmpsModifier;
        _CurrentWheelLeftSpeed -= _CubeShakeWheelSpeedMmpsModifier;
      }
      else {
        if (_CurrentWheelRightSpeed > 0 || _CurrentWheelLeftSpeed < 0) {
          _CurrentWheelRightSpeed = 0;
          _CurrentWheelLeftSpeed = 0;
        }
        _CurrentWheelRightSpeed -= _CubeShakeWheelSpeedMmpsModifier;
        _CurrentWheelLeftSpeed += _CubeShakeWheelSpeedMmpsModifier;
      }
    }

    private void ChangeRotateCubeState() {
      if (_RotateCubeState == RotateCubeState.LEFT) {
        _RotateCubeState = RotateCubeState.RIGHT;
      }
      else {
        _RotateCubeState = RotateCubeState.LEFT;
      }
      _LastRotateCubeStateChanged = Time.time;
    }
  }
}


