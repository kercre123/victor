using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CodeBreaker {
  public class WaitForGuessState : State {
    private const float kAdjustDistanceDriveMmps = 40f;
    private const float kCubeTooCloseDistanceThreshold = 140f;
    private const float kCubeTooFarDistanceThreshold = 190f;
    private const float kCubeTooHighDistanceThreshold = 50f;

    private const float kTargetAreaBuffer = 25f;

    private CubeCode _WinningCode;
    private CodeBreakerGame _Game;
    private List<LightCube> _CubesInRow;

    public WaitForGuessState(CubeCode winningCubeCode) {
      _WinningCode = winningCubeCode;
    }

    public override void Enter() {
      _Game = _StateMachine.GetGame() as CodeBreakerGame;

      // TODO: Play intermittent "impatient" animation? Use mood manager?
      _CurrentRobot.SetIdleAnimation("_LIVE_");

      _Game.ShowGamePanel(HandleSubmitButtonClicked);
      _Game.EnableSubmitButton = false;
    }

    private void HandleSubmitButtonClicked() {
      CubeCode guess = new CubeCode();

      // TODO: Evaluate guess from cubes in row
      _StateMachine.SetNextState(new EvaluateGuessState(_WinningCode, guess));
    }

    public override void Update() {
      base.Update();

      // --------------------------------
      // TODO: Potentially do a track to object? / tilt head / engine behavior for this instead
      // --------------------------------
      bool didDrive = false;
      TryDriveTowardsClosestObject();

      // For now always do this check; used to be on !didDrive
      // however, the submit button would flicker a lot because of his idle animation?
      bool shouldEnableSubmitButton = false;
      if (!didDrive) {
        // TODO: Handle more than the required cubes (objects in background);
        if (_CurrentRobot.VisibleObjects.Count == _WinningCode.NumCubes) {
          List<LightCube> visibleCubes = GetVisibleLightCubes();
          if (visibleCubes.Count == _WinningCode.NumCubes) {
            float targetY = _WinningCode.NumCubes * CozmoUtil.kBlockLengthMM * 0.5f + kTargetAreaBuffer;
            Vector3 testCubeCozmoPos;
            bool cubesCloseToCozmo = true;
            foreach (var cube in visibleCubes) {
              testCubeCozmoPos = _CurrentRobot.WorldToCozmo(cube.WorldPosition);
              // TODO: Depend on row check, no proximity check?
              // is cube close enough to the center of cozmo's view?
              if (!(testCubeCozmoPos.y <= targetY && testCubeCozmoPos.y >= -targetY
                  && testCubeCozmoPos.z <= kCubeTooHighDistanceThreshold)) {
                cubesCloseToCozmo = false;
                break;
              }
            }

            if (cubesCloseToCozmo && CheckRowAlignment(visibleCubes)) {
              shouldEnableSubmitButton = true;
            }
          }
        }
      }

      _Game.EnableSubmitButton = shouldEnableSubmitButton;
    }

    private bool TryDriveTowardsClosestObject() {

      bool didDrive = true;
      // Grab the closest cube, if it's too close or too far, move away
      float closestDistance = GetClosestObjectDistance(_CurrentRobot.VisibleObjects);
      if (closestDistance < kCubeTooCloseDistanceThreshold) {
        _CurrentRobot.DriveWheels(-kAdjustDistanceDriveMmps, -kAdjustDistanceDriveMmps);
      }
      else if (closestDistance > kCubeTooFarDistanceThreshold && closestDistance != float.MaxValue) {
        _CurrentRobot.DriveWheels(kAdjustDistanceDriveMmps, kAdjustDistanceDriveMmps);
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        didDrive = false;
      }
      return didDrive;
    }

    private float GetClosestObjectDistance(List<ObservedObject> objs) {
      float distance = float.MaxValue;
      foreach (var obj in objs) {
        float d = Vector3.Distance(_CurrentRobot.WorldPosition, obj.WorldPosition);
        if (d < distance) {
          distance = d;
        }
      }
      return distance;
    }

    private List<LightCube> GetVisibleLightCubes() {
      // Get the cubes
      List<LightCube> visibleCubes = new List<LightCube>();
      foreach (var obj in _CurrentRobot.VisibleObjects) {
        if (obj is LightCube) {
          visibleCubes.Add(obj as LightCube);
        }
      }
      return visibleCubes;
    }

    // TODO: Better row checking
    private bool CheckRowAlignment(List<LightCube> cubes) {
      bool isRow = true;
      for (int i = 0; i < cubes.Count - 1; ++i) {
        Vector3 obj0to1 = cubes[i + 1].WorldPosition - cubes[i].WorldPosition;
        obj0to1.Normalize();
        if (Vector3.Dot(obj0to1, _CurrentRobot.Forward) > 0.1f) {
          isRow = false;
          break;
        }
      }
      return isRow;
    }
  }
}
