using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace StackTraining {
  public class HelpCozmoStackState : State {

    private StackTrainingGame _Game;

    private float _MovingTime = 0f;

    private float _Steering;

    private bool _HasSeenCube;

    private bool _Moving;

    private bool _Carrying;

    private float _InvisibleBlockTime = 0f;

    private Bounds _TouchingBounds = new Bounds(new Vector3(30, 0, 20), new Vector3(10, 20, 20));

    private Vector3 _StartPosition;
    private Quaternion _StartRotation;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as StackTrainingGame;
      _Carrying = true;

      _Game.ShowHowToPlaySlide("HelpStack");
      RobotEngineManager.Instance.OnObservedMotion += HandleDetectMotion;
      _StartPosition = _CurrentRobot.WorldPosition;
      _StartRotation = _CurrentRobot.Rotation;
    }

    public override void Update() {
      base.Update();

      var bottomCube = _Game.BottomCube;
      var topCube = _Game.TopCube;

      if (bottomCube == null || topCube == null) {
        _Game.RaiseMiniGameLose();
        return;
      }

      if (_Moving) {
        return;
      }

      if (!_Carrying) {

        if (_InvisibleBlockTime < 2f || (!topCube.MarkersVisible && _InvisibleBlockTime < 6f)) {
          _InvisibleBlockTime += Time.deltaTime;
          _CurrentRobot.DriveWheels(-25f, -25f);
          // raise the head so we can check the block has been placed
          _CurrentRobot.SetHeadAngle(0.25f);
        }
        else {
          _CurrentRobot.DriveWheels(0, 0);

          if (CubesStacked()) {
            _Game.Progress = 1f;
            HandleComplete();
          }
          else {
            if (_Game.TryDecrementAttempts()) {
              _StateMachine.SetNextState(new HelpCozmoPickupState());
            }
            else {
              HandleFailed();
            }
          }
        }

        return;
      }

      if (!CubeInPlacementRange()) {        
        if (topCube.MarkersVisible) {
          topCube.SetLEDs(Color.white);
          _InvisibleBlockTime = 0f;
          _HasSeenCube = true;
        }
        else {
          topCube.SetLEDs(Color.red);

          _InvisibleBlockTime += Time.deltaTime;

          if (_InvisibleBlockTime > 10f && _HasSeenCube) {
            if (_Game.TryDecrementAttempts()) {
              _InvisibleBlockTime = 0f;
              _HasSeenCube = false;

              _Moving = true;
              // try to go back to our starting position
              _CurrentRobot.GotoPose(_StartPosition, _StartRotation, (s) => {
                _Moving = false;
              });
            }
            else {
              HandleFailed();
            }
          }
        }

        // 475 == 1, 75 == 0
        float scaledDistance = ((topCube.WorldPosition - _CurrentRobot.WorldPosition).magnitude - 75) * 0.0025f;

        _Game.Progress = 1 - 0.2f * Mathf.Clamp01(scaledDistance);

        if (_MovingTime > 0f) {
          _MovingTime -= Time.deltaTime;
          _CurrentRobot.DriveWheels(Mathf.Lerp(15f, 25f, _Steering), Mathf.Lerp(25f, 15f, _Steering));
        }
        else {
          _CurrentRobot.DriveWheels(0, 0);
        }
      }
      else {
        if (TouchingTopCube()) {
          // move backwards slowly as we lower the lift to place the cube
          _CurrentRobot.DriveWheels(-25f, -25f);
          _CurrentRobot.SetLiftHeight(0.1f);
          _Carrying = false;
          _InvisibleBlockTime = 0f;
        }
        else {
          _CurrentRobot.DriveWheels(20f, 20f);
        }
      }
    }

    private bool CubesStacked() {
      var bottomCube = _Game.BottomCube;
      var topCube = _Game.TopCube;

      Vector3 bottomCubePosition = _CurrentRobot.WorldToCozmo(bottomCube.WorldPosition);
      Vector3 topCubePosition = _CurrentRobot.WorldToCozmo(topCube.WorldPosition);
      Vector2 xyDelta = topCubePosition - bottomCubePosition;

      return (Mathf.Abs(xyDelta.x) < 15f && Mathf.Abs(xyDelta.y) < 15f);
    }

    private bool TouchingTopCube() {
      var topCube = _Game.TopCube;

      Vector3 topCubePosition = _CurrentRobot.WorldToCozmo(topCube.WorldPosition);

      return _TouchingBounds.Contains(topCubePosition);
    }

    private bool CubeInPlacementRange() {
      var topCube = _Game.TopCube;

      Vector3 topCubePosition = _CurrentRobot.WorldToCozmo(topCube.WorldPosition);

      // check that cube is withing N units of cozmo and its y is centered
      const float placementRange = 75f;
      return (topCubePosition.sqrMagnitude < placementRange * placementRange &&
      Mathf.Abs(topCubePosition.y) < 10f);
    }

    private void HandleDetectMotion(Vector2 position) {
      // position goes from -1 to 1, _Steering goes from 0 to 1
      _Steering = position.x * 0.5f + 0.5f;
      _MovingTime = 3f;
    }

    private void HandleComplete() {
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kMajorWin, HandleWinComplete);
      _StateMachine.SetNextState(animState);
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.white, 100, 100, 0);
      }
    }

    private void HandleFailed() {
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kShocked, HandleLoseComplete);
      _StateMachine.SetNextState(animState);
    }

    private void HandleWinComplete(bool success) {
      _Game.RaiseMiniGameWin();
    }

    private void HandleLoseComplete(bool success) {
      _Game.RaiseMiniGameLose();
    }

    public override void Exit() {
      base.Exit();
      RobotEngineManager.Instance.OnObservedMotion -= HandleDetectMotion;
    }

  }

}
