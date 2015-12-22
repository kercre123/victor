using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace Minesweeper {
  public class SetupGridState : State {

    private int _CurrentIndex;
    private int[] _CubeIds;

    private const float kCellWidth = 100f;

    private Vector3[] _CubePositions;

    private Vector3 _CenterPosition;

    public override void Enter() {
      base.Enter();
      _CubeIds = _CurrentRobot.LightCubes.Keys.ToArray();
      _CurrentIndex = -1;

      foreach (var cube in _CurrentRobot.LightCubes.Values) {
        cube.TurnLEDsOff();
      }

      var game = (MinesweeperGame)_StateMachine.GetGame();

      var topRightCorner = new Vector3(game.Rows / 2f, game.Columns / 2f, 0) * kCellWidth;
      var bottomRightCorner = new Vector3(topRightCorner.x, -topRightCorner.y, 0);
      var topLeftCorner = -bottomRightCorner;
      var bottomLeftCorner = -topRightCorner;

      _CenterPosition = _CurrentRobot.WorldPosition;
      _CubePositions = new Vector3[4] { 
        topLeftCorner + _CurrentRobot.WorldPosition,
        bottomRightCorner + _CurrentRobot.WorldPosition,
        topRightCorner + _CurrentRobot.WorldPosition,
        bottomLeftCorner + _CurrentRobot.WorldPosition
      };     
    }

    public override void Update() {
      base.Update();
      _CurrentIndex++;

      if (_CurrentIndex >= _CubeIds.Length || _CurrentIndex >= _CubePositions.Length) {
        // GOTO Play State
        //_StateMachine.SetNextState();
        return;
      }

      _StateMachine.PushSubState(new GoToPlacementPoseState(_CenterPosition, _CubePositions[_CurrentIndex], _CubeIds[_CurrentIndex]));
    }

    public override void Exit() {
      base.Exit();
    }
  }


  public class GoToPlacementPoseState : State {

    private Vector3 _Position;
    private int _CubeId;
    private Vector3 _Center;

    public GoToPlacementPoseState(Vector3 center, Vector3 position, int cubeId) {
      _Position = position;
      _CubeId = cubeId;
      _Center = center;
    }

    public override void Enter() {
      base.Enter();
      var delta = _Position - _Center;

      var deltaNorm = delta.normalized;
      var angle = Mathf.Atan2(delta.y, delta.x) * Mathf.Rad2Deg;

      var rotation = Quaternion.Euler(0, 0, angle);

      _CurrentRobot.GotoPose(_Center + delta - deltaNorm * 100f, rotation, HandleRotateComplete);
    }

    protected void HandleRotateComplete(bool success) {
      _StateMachine.SetNextState(new WaitForCubeState(_Position, _CubeId));
    }
  }


  public class WaitForCubeState : State {
    private int _CubeId;

    private Bounds _Bounds;
    private float _InBoundsTime = 0f;

    private const float kPositionTolerance = 30f;

    public WaitForCubeState(Vector3 position, int cubeId) {
      _Bounds = new Bounds(position + Vector3.forward * 0.5f * CozmoUtil.kBlockLengthMM, Vector3.one * kPositionTolerance);
      _CubeId = cubeId;
    }

    public override void Update() {
      base.Update();

      var cube = _CurrentRobot.LightCubes[_CubeId];

      if (_Bounds.Contains(cube.WorldPosition)) {
        cube.SetLEDs(Color.blue);
        _InBoundsTime += Time.deltaTime;
        if (_InBoundsTime > 3f) {
          _StateMachine.SetNextState(new TapCubeState(null, _CubeId, HandleCubeTapped));
        }
      }
      else {
        _InBoundsTime = 0f;
        cube.SetLEDs(Color.red);
      }
    }

    private void HandleCubeTapped() {
      _StateMachine.PopState();
    }
  }
}