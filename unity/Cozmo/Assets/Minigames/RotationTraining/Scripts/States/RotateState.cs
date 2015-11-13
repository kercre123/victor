using UnityEngine;
using System.Timers;
using System.Collections;
using System.Collections.Generic;

namespace RotationTraining {
  public class RotateState : State {

    public enum RotateCubeState {
      Left,
      Right
    }

    private int _TurningCubeID = -1;
    private LightCube _TurningCube;
    private Color[] _RotateCubeColors = { Color.blue, Color.green };
    private RotateCubeState _RotateCubeState = RotateCubeState.Left;

    private float _NextCubeChangeTime;

    // TODO: Get these from data
    private float _MinChangeSeconds = 3f;
    private float _MaxChangeSeconds = 5f;

    private uint _FlashCubeLightMs = 500;
    private bool _CubeIsFlashing = false;
    private Timer _FlashCubeTimer;

    private float _CurrentWheelLeftSpeed = 0.0f;
    private float _CurrentWheelRightSpeed = 0.0f;

    private float _CubeShakeWheelSpeedMmpsModifier = 300f;

    // Value from 0 to 1
    private float _WheelSpeedDecayModifier = 0.75f;

    public override void Enter() {
      base.Enter();
      LightCube.MovedAction += OnCubeMoved;

      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      // PoseAngle gets reset to 0 automatically on state enter.
      // Range is -PI to PI
      // _CurrentRobot.PoseAngle = 0;

      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        _TurningCubeID = kvp.Key;
        _TurningCube = kvp.Value;
        break;
      }

      // Pick a cube state randomly
      _RotateCubeState = Random.Range(0.0f, 1.0f) > 0.5f ? RotateCubeState.Left : RotateCubeState.Right;
      _TurningCube.SetLEDs(_RotateCubeColors[(int)_RotateCubeState]);

      // Set the next change time to sometime in the future.
      _NextCubeChangeTime = GenerateNextChangeTime();

      // Set up flashing cube timer.
      _FlashCubeTimer = new Timer(_FlashCubeLightMs);
      _FlashCubeTimer.AutoReset = false;
      _FlashCubeTimer.Elapsed += HandleFlashCubeTimerStopped;
      _FlashCubeTimer.Stop();

      // TODO: Start timer for lose state
    }

    public override void Update() {

      base.Update();

      // Change the cube light if it's time.
      if (!_CubeIsFlashing && Time.time > _NextCubeChangeTime) {
        StartFlashingCube();
      }

      // celebrate when he is at 180... TODO: don't let players cheat by rotating cozmo manually?
      float marginOfError = 0.9f;
      if (_CurrentRobot.PoseAngle > Mathf.PI * marginOfError || _CurrentRobot.PoseAngle < -Mathf.PI * marginOfError) {
        _StateMachine.SetNextState(new CelebrateState());
      }

      // Decay the wheel speed over time if the cube doesn't move
      _CurrentWheelLeftSpeed = Mathf.Lerp(_CurrentWheelLeftSpeed, 0.0f, _WheelSpeedDecayModifier * Time.deltaTime);
      _CurrentWheelRightSpeed = Mathf.Lerp(_CurrentWheelRightSpeed, 0.0f, _WheelSpeedDecayModifier * Time.deltaTime);

      _CurrentRobot.DriveWheels(_CurrentWheelLeftSpeed * Time.deltaTime, _CurrentWheelRightSpeed * Time.deltaTime);
    }

    public override void Exit() {
      base.Exit();
    }

    private void OnCubeMoved(int cubeID, float x, float y, float z) {

      if (cubeID != _TurningCubeID) {
        return;
      }

      if (_RotateCubeState == RotateCubeState.Left) {
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

    private void StartFlashingCube() {
      _CubeIsFlashing = true;
      _FlashCubeTimer.Start();

      uint flashDuration = _FlashCubeLightMs / 7;
      _TurningCube.SetFlashingLEDs(_RotateCubeColors[(int)_RotateCubeState], flashDuration, flashDuration, 0);
    }

    private void ChangeRotateCubeState() {
      if (_RotateCubeState == RotateCubeState.Left) {
        _RotateCubeState = RotateCubeState.Right;
      }
      else {
        _RotateCubeState = RotateCubeState.Left;
      }
      _NextCubeChangeTime = GenerateNextChangeTime();

      // set light cube color based on _RotateCubeState
      _TurningCube.SetLEDs(_RotateCubeColors[(int)_RotateCubeState]);
    }

    private float GenerateNextChangeTime() {
      return Time.time + Random.Range(_MinChangeSeconds, _MaxChangeSeconds);
    }

    private void HandleFlashCubeTimerStopped(object source, ElapsedEventArgs e) {
      _CubeIsFlashing = false;
      ChangeRotateCubeState();
    }
  }
}


