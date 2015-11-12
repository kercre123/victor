using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace RotationTraining {
  public class RotateState : State {

    public enum RotateCubeState {
      LEFT,
      RIGHT
    }

    private Color[] _RotateCubeColors = { Color.blue, Color.green };
    private RotateCubeState _RotateCubeState = RotateCubeState.LEFT;
    private float _LastRotateCubeStateChanged;
    private float _InitialPoseRadian;
    private int _CubeID = -1;

    private float _DriveWheelLeftSpeed = 0.0f;
    private float _DriveWheelRightSpeed = 0.0f;

    public override void Enter() {
      base.Enter();
      _LastRotateCubeStateChanged = Time.time;
      LightCube.MovedAction += OnCubeMoved;

      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        _CubeID = kvp.Key;
        break;
      }
    }

    public override void Update() {
      base.Update();
      // TODO: Use better change method
      if (_LastRotateCubeStateChanged > 2.0f && Random.Range(0.0f, 1.0f) > 0.98f) {
        ChangeRotateCubeState();
      }

      // celebrate when he is at 180... TODO: don't let players cheat by rotating cozmo manually?
      if (_CurrentRobot.PoseAngle > 0.9f * Mathf.PI && _CurrentRobot.PoseAngle < 1.1f * Mathf.PI) {
        _StateMachine.SetNextState(new CelebrateState());
      }

      // set light cube color based on _RotateCubeState
      _CurrentRobot.LightCubes[_CubeID].SetLEDs(CozmoPalette.ColorToUInt(_RotateCubeColors[(int)_RotateCubeState]));

      _DriveWheelLeftSpeed = Mathf.Lerp(_DriveWheelLeftSpeed, 0.0f, Time.deltaTime * 8.0f);
      _DriveWheelRightSpeed = Mathf.Lerp(_DriveWheelRightSpeed, 0.0f, Time.deltaTime * 8.0f);
      _CurrentRobot.DriveWheels(_DriveWheelLeftSpeed, _DriveWheelRightSpeed);

    }

    public override void Exit() {
      base.Exit();
    }

    private void OnCubeMoved(int cubeID, float x, float y, float z) {

      if (cubeID != _CubeID) {
        return;
      }

      if (_RotateCubeState == RotateCubeState.LEFT) {
        _DriveWheelRightSpeed += Time.deltaTime * 5000.0f;
        _DriveWheelLeftSpeed -= Time.deltaTime * 5000.0f;
      }
      else {
        _DriveWheelLeftSpeed += Time.deltaTime * 5000.0f;
        _DriveWheelRightSpeed -= Time.deltaTime * 5000.0f;
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


