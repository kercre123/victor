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

    #region Tunable values

    // TODO: Get these from data
    private float _MinCubeChangeSeconds = 3f;
    private float _MaxCubeChangeSeconds = 5f;

    private uint _FlashCubeLightMs = 700;

    private float _CubeShakeWheelSpeedMmpsModifier = 300f;

    // Value from 0 to 1
    private float _WheelSpeedDecayModifier = 0.75f;

    private float _MarginOfErrorRad = 0.1f;

    private int _GameTimerDurationSeconds = 30;

    #endregion

    #region State Variables

    private RotationTrainingGame _GameInstance;
    private int _TurningCubeID = -1;
    private LightCube _TurningCube;
    private RotateCubeState _RotateCubeState = RotateCubeState.Left;

    private float _NextCubeChangeTimestamp;

    private bool _CubeIsFlashing = false;
    private Timer _FlashCubeTimer;

    private float _CurrentWheelLeftSpeed = 0.0f;
    private float _CurrentWheelRightSpeed = 0.0f;

    private float _TargetPoseAngleMinRad;
    private float _TargetPoseAngleMaxRad;

    private float _FailGameTimestamp;
    private int _SecondsRemaining;

    #endregion

    public override void Enter() {
      base.Enter();

      // Adjust robot
      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      // Register cube movement
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        _TurningCubeID = kvp.Key;
        _TurningCube = kvp.Value;
        break;
      }
      LightCube.MovedAction += OnCubeMoved;

      // Pick a cube state randomly
      _RotateCubeState = Random.Range(0.0f, 1.0f) > 0.5f ? RotateCubeState.Left : RotateCubeState.Right;
      UpdateRobotLEDs();

      // Set the next change time to sometime in the future.
      _NextCubeChangeTimestamp = GenerateNextChangeTime();

      // Set up flashing cube timer.
      _FlashCubeTimer = new Timer(_FlashCubeLightMs);
      _FlashCubeTimer.AutoReset = false;
      _FlashCubeTimer.Elapsed += HandleFlashCubeTimerStopped;
      _FlashCubeTimer.Stop();

      // Select target angle
      SetUpTargetAngle();

      // Start timer for lose state
      _SecondsRemaining = _GameTimerDurationSeconds;
      _FailGameTimestamp = Time.time + _GameTimerDurationSeconds;
      _GameInstance = _StateMachine.GetGame() as RotationTrainingGame;
      _GameInstance.SetTimeLeft(_SecondsRemaining);
    }

    public override void Update() {

      base.Update();

      // Change the cube light if it's time.
      if (!_CubeIsFlashing && Time.time > _NextCubeChangeTimestamp) {
        StartFlashingCube();
        _NextCubeChangeTimestamp = GenerateNextChangeTime();
      }

      // celebrate when he is at 180... TODO: don't let players cheat by rotating cozmo manually?
      bool inTarget = CheckIfInTargetAngle();
      if (inTarget) {
        _StateMachine.SetNextState(new CelebrateState());

        // Stop moving
        _CurrentRobot.DriveWheels(0, 0);
      }
      else {
        // Decay the wheel speed over time if the cube doesn't move
        _CurrentWheelLeftSpeed = Mathf.Lerp(_CurrentWheelLeftSpeed, 0.0f, _WheelSpeedDecayModifier * Time.deltaTime);
        _CurrentWheelRightSpeed = Mathf.Lerp(_CurrentWheelRightSpeed, 0.0f, _WheelSpeedDecayModifier * Time.deltaTime);

        _CurrentRobot.DriveWheels(_CurrentWheelLeftSpeed * Time.deltaTime, _CurrentWheelRightSpeed * Time.deltaTime);

        // Update the timer
        int newSecondsLeft = Mathf.FloorToInt(_FailGameTimestamp - Time.time);
        if (newSecondsLeft != _SecondsRemaining) {
          _SecondsRemaining = newSecondsLeft;
          _GameInstance.SetTimeLeft(_SecondsRemaining);
        }
      }
    }

    public override void Exit() {
      base.Exit();
    }

    private void OnCubeMoved(int cubeID, float x, float y, float z) {

      if (cubeID != _TurningCubeID) {
        return;
      }

      if (_RotateCubeState == RotateCubeState.Left) {
        // If cozmo was turning the other way, reset his wheels
        if (_CurrentWheelRightSpeed < 0 || _CurrentWheelLeftSpeed > 0) {
          _CurrentWheelRightSpeed = 0;
          _CurrentWheelLeftSpeed = 0;
        }
        // Increase wheel speed with each shake
        _CurrentWheelRightSpeed += _CubeShakeWheelSpeedMmpsModifier;
        _CurrentWheelLeftSpeed -= _CubeShakeWheelSpeedMmpsModifier;
      }
      else {
        // If cozmo was turning the other way, reset his wheels
        if (_CurrentWheelRightSpeed > 0 || _CurrentWheelLeftSpeed < 0) {
          _CurrentWheelRightSpeed = 0;
          _CurrentWheelLeftSpeed = 0;
        }
        // Increase wheel speed with each shake
        _CurrentWheelRightSpeed -= _CubeShakeWheelSpeedMmpsModifier;
        _CurrentWheelLeftSpeed += _CubeShakeWheelSpeedMmpsModifier;
      }
    }

    private void StartFlashingCube() {
      _CubeIsFlashing = true;
      _FlashCubeTimer.Start();

      uint flashDuration = _FlashCubeLightMs / 7;
      Color currentColor = GetRotateLightColor();
      _TurningCube.SetFlashingLEDs(currentColor, flashDuration, flashDuration, 0);

      Anki.Cozmo.LEDId currentLedId = GetRotateLEDId();
      _CurrentRobot.SetFlashingBackpackLED(currentLedId, currentColor, flashDuration, flashDuration, 0);
    }

    private void ChangeRotateCubeState() {
      if (_RotateCubeState == RotateCubeState.Left) {
        _RotateCubeState = RotateCubeState.Right;
      }
      else {
        _RotateCubeState = RotateCubeState.Left;
      }
      UpdateRobotLEDs();
    }

    private void UpdateRobotLEDs() {
      // set light cube color based on _RotateCubeState
      Color rotateColor = GetRotateLightColor();
      _TurningCube.SetLEDs(rotateColor);
      _CurrentRobot.SetBackpackLED(GetRotateLEDId(), rotateColor);
    }

    private float GenerateNextChangeTime() {
      return Time.time + Random.Range(_MinCubeChangeSeconds, _MaxCubeChangeSeconds);
    }

    private void HandleFlashCubeTimerStopped(object source, ElapsedEventArgs e) {
      _CubeIsFlashing = false;
      ChangeRotateCubeState();
    }

    private Color GetRotateLightColor() {
      Color rotateColor = Color.white;
      switch (_RotateCubeState) {
      case RotateCubeState.Left:
        rotateColor = Color.green;
        break;
      case RotateCubeState.Right:
        rotateColor = Color.blue;
        break;
      default:
        break;
      }
      return rotateColor;
    }

    private Anki.Cozmo.LEDId GetRotateLEDId() {
      Anki.Cozmo.LEDId ledId = Anki.Cozmo.LEDId.LED_BACKPACK_RIGHT;
      switch (_RotateCubeState) {
      case RotateCubeState.Left:
        ledId = Anki.Cozmo.LEDId.LED_BACKPACK_LEFT;
        break;
      case RotateCubeState.Right:
        ledId = Anki.Cozmo.LEDId.LED_BACKPACK_RIGHT;
        break;
      default:
        break;
      }
      return ledId;
    }

    private void SetUpTargetAngle() {
      // Get the target
      float targetRadian = _CurrentRobot.PoseAngle + Mathf.PI;

      // Offset the target by some amount so it's easier for the player
      _TargetPoseAngleMaxRad = targetRadian + _MarginOfErrorRad;
      _TargetPoseAngleMinRad = targetRadian - _MarginOfErrorRad;

      // If range is over the pose angle limit, overflow to the other side
      if (_TargetPoseAngleMaxRad > Mathf.PI) {
        float diff = _TargetPoseAngleMaxRad - Mathf.PI;
        _TargetPoseAngleMaxRad = -Mathf.PI + diff;
      }
      if (_TargetPoseAngleMinRad > Mathf.PI) {
        float diff = _TargetPoseAngleMinRad - Mathf.PI;
        _TargetPoseAngleMinRad = -Mathf.PI + diff;        
      }
    }

    private bool CheckIfInTargetAngle() {
      // Special case if the target range is near the limit
      bool inTarget = false;
      if (_TargetPoseAngleMinRad > _TargetPoseAngleMaxRad) {
        inTarget = (_CurrentRobot.PoseAngle > _TargetPoseAngleMinRad || _CurrentRobot.PoseAngle < _TargetPoseAngleMaxRad);
      }
      else {
        inTarget = (_CurrentRobot.PoseAngle > _TargetPoseAngleMinRad && _CurrentRobot.PoseAngle < _TargetPoseAngleMaxRad);
      }
      return inTarget;
    }
  }
}


