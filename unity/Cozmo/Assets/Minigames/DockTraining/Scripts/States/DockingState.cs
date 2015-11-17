using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DockTraining {

  public class DockingState : State {
    
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

    private float _DriveWheelRandMin = -10.0f;
    private float _DriveWheelRandMax = 45.0f;


    DockTrainingGame _DockTrainingGame;
    LightCube _CurrentTarget = null;

    public override void Enter() {
      base.Enter();
      _LastRotateCubeStateChanged = Time.time;
      LightCube.OnMovedAction += HandleCubeMoved;

      _CurrentRobot.SetHeadAngle(-1.0f);
      _CurrentRobot.SetLiftHeight(0.0f);

      _DockTrainingGame = _StateMachine.GetGame() as DockTrainingGame;
      _CurrentTarget = _DockTrainingGame.GetCurrentTarget();
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        if (_CurrentTarget.ID == kvp.Key) {
          continue;
        }
        _CubeID = kvp.Key;
        break;
      }
    }

    public override void Update() {
      base.Update();

      LightCube target = _DockTrainingGame.GetCurrentTarget();

      if (target == null || target != _CurrentTarget) {
        _StateMachine.SetNextState(new AngryTargetChangedState());
      }
      else {
        float distance = Vector3.Distance(_CurrentRobot.WorldPosition, _CurrentTarget.WorldPosition);
        float relDot = Vector2.Dot((Vector2)(_CurrentRobot.Forward).normalized, (Vector2)((_CurrentTarget.WorldPosition - _CurrentRobot.WorldPosition).normalized));

        Debug.Log(distance);
        Debug.Log(relDot);

        if (distance < 60.0f) {
          _DriveWheelRandMin = 0.0f;
          _DriveWheelRandMax = 20.0f;
        }
        else {
          _DriveWheelRandMin = -10.0f;
          _DriveWheelRandMax = 45.0f;
        }

        if (distance < 53.0f && relDot > 0.88f && _CurrentRobot.VisibleObjects.Contains(_CurrentTarget)) {
          _StateMachine.SetNextState(new CelebrateState());
        }
        else {
          SteeringLogic();
        }

      }

    }

    public override void Exit() {
      base.Exit();
    }

    private void SteeringLogic() {
      // TODO: Use better change method
      if (_LastRotateCubeStateChanged > 2.0f && Random.Range(0.0f, 1.0f) > 0.98f) {
        ChangeRotateCubeState();
      }

      // set light cube color based on _RotateCubeState
      _CurrentRobot.LightCubes[_CubeID].SetLEDs(CozmoPalette.ColorToUInt(_RotateCubeColors[(int)_RotateCubeState]));

      _DriveWheelLeftSpeed = Mathf.Lerp(_DriveWheelLeftSpeed, Random.Range(_DriveWheelRandMin, _DriveWheelRandMax), Time.deltaTime * 10.0f);
      _DriveWheelRightSpeed = Mathf.Lerp(_DriveWheelRightSpeed, Random.Range(_DriveWheelRandMin, _DriveWheelRandMax), Time.deltaTime * 10.0f);
      _CurrentRobot.DriveWheels(_DriveWheelLeftSpeed, _DriveWheelRightSpeed);
      //_CurrentRobot.DriveWheels(Random.Range(-10.0f, 45.0f), Random.Range(-10.0f, 45.0f));
    }

    private void HandleCubeMoved(int cubeID, float x, float y, float z) {

      if (cubeID != _CubeID) {
        return;
      }

      if (_RotateCubeState == RotateCubeState.LEFT) {
        _DriveWheelRightSpeed += Time.deltaTime * 1000.0f;
        _DriveWheelLeftSpeed -= Time.deltaTime * 1000.0f;
      }
      else {
        _DriveWheelLeftSpeed += Time.deltaTime * 1000.0f;
        _DriveWheelRightSpeed -= Time.deltaTime * 1000.0f;
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
