using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace CodeBreaker {
  public class WaitForGuessState : State {
    private const float kAdjustDistanceDriveMmps = 40f;
    private const float kCubeDistance = 150f;
    private const float kCubetooCloseDistance = 100f;
    private const float kCubeTooHighDistanceThreshold = 50f;
    private const float kDriveToCubeBuffer = 25f;
    private const float kTargetAreaBuffer = 25f;
    private const float kSecondSinceLastCubeThreshold = 1f;

    private CubeCode _WinningCode;
    private CodeBreakerGame _Game;

    private CubeState[] _TargetCubeStates;

    private Color[] _ValidCodeColors;

    private float _SecondsSinceLastCubeAdjust = 0f;
    private bool _IsDrivingToPose = false;

    #region State Upkeep

    public WaitForGuessState(CubeCode winningCubeCode, LightCube[] targetCubes) {
      // Foreach cube create a cube state
      CubeState[] targetCubeStates = new CubeState[targetCubes.Length];
      for (int i = 0; i < targetCubes.Length; i++) {
        targetCubeStates[i] = new CubeState();
        targetCubeStates[i].cube = targetCubes[i];
        targetCubeStates[i].currentColorIndex = 0;
      }

      Initialize(winningCubeCode, targetCubeStates);
    }

    public WaitForGuessState(CubeCode winningCubeCode, CubeState[] targetCubeStates) {
      Initialize(winningCubeCode, targetCubeStates);
    }

    private void Initialize(CubeCode winningCubeCode, CubeState[] targetCubeStates) {
      _WinningCode = winningCubeCode;
      _TargetCubeStates = targetCubeStates;

      // Add a listener to each cube
      LightCube.TappedAction += OnBlockTapped;
    }

    public override void Enter() {
      _Game = _StateMachine.GetGame() as CodeBreakerGame;
      SetConstrictedLiveAnimation();

      _Game.ShowGamePanel(HandleSubmitButtonClicked);
      _Game.EnableSubmitButton = false;
      _Game.UpdateGuessesUI();

      _ValidCodeColors = _Game.ValidColors;
      foreach (var cubeState in _TargetCubeStates) {
        cubeState.cube.SetLEDsOff();
        cubeState.cube.SetLEDs(_ValidCodeColors[cubeState.currentColorIndex]);
      }
      _SecondsSinceLastCubeAdjust = 0f;
    }

    public override void Update() {
      base.Update();

      if (!_IsDrivingToPose) {
        _SecondsSinceLastCubeAdjust += Time.deltaTime;
      }
      bool shouldEnableSubmitButton = false;
      List<LightCube> visibleCubes = GetVisibleLightCubes();
      if (visibleCubes.Count > 1) {
        // Drive towards midpoint of visible cubes
        if (!_IsDrivingToPose) {
          if (!TryDriveAwayFromTooCloseObject()
              && _SecondsSinceLastCubeAdjust > kSecondSinceLastCubeThreshold) {
            DriveTowardsCubeMidpoint(visibleCubes);
          }

          if (visibleCubes.Count == _WinningCode.NumCubes) {
            shouldEnableSubmitButton = ShouldEnableSubmitButton(visibleCubes);
          }
        }
      }
      else if (visibleCubes.Count == 1) {
        if (!_IsDrivingToPose && !TryDriveAwayFromTooCloseObject()
            && _SecondsSinceLastCubeAdjust > kSecondSinceLastCubeThreshold) {
          DriveToCube(visibleCubes[0]);
        }
      }

      _Game.EnableSubmitButton = shouldEnableSubmitButton;
    }

    public override void Exit() {
      base.Exit();
      _Game.RemoveSubmitButtonListener(HandleSubmitButtonClicked);
      LightCube.TappedAction -= OnBlockTapped;
    }

    #endregion

    #region Event handling

    private void OnBlockTapped(int id, int times, float timeStamp) {
      // If the id matches change the index and color, depending on the number of times tapped
      GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
      foreach (var cubeState in _TargetCubeStates) {
        if (cubeState.cube.ID == id) {
          cubeState.currentColorIndex += times;
          cubeState.currentColorIndex %= _ValidCodeColors.Length;
          cubeState.cube.SetLEDs(_ValidCodeColors[cubeState.currentColorIndex]);
          break;
        }
      }
    }

    private void HandleSubmitButtonClicked() {
      _Game.UseGuess();
      _StateMachine.SetNextState(new EvaluateGuessState(_WinningCode, _TargetCubeStates));
    }

    #endregion

    private bool TryDriveAwayFromTooCloseObject() {
      bool didDrive = true;
      // Grab the closest cube, if it's too close or too far, move away
      float closestDistance = GetClosestObjectDistance(_TargetCubeStates);
      if (closestDistance < kCubetooCloseDistance) {
        _CurrentRobot.DriveWheels(-kAdjustDistanceDriveMmps, -kAdjustDistanceDriveMmps);
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        didDrive = false;
      }
      return didDrive;
    }

    private float GetClosestObjectDistance(CubeState[] cubeStates) {
      float distance = float.MaxValue;
      foreach (var cubeState in cubeStates) {
        float d = Vector3.Distance(_CurrentRobot.WorldPosition, cubeState.cube.WorldPosition);
        if (d < distance) {
          distance = d;
        }
      }
      return distance;
    }

    private List<LightCube> GetVisibleLightCubes() {
      // Get the cubes
      List<LightCube> visibleCubes = new List<LightCube>();
      foreach (var obj in _TargetCubeStates) {
        if (obj.cube.IsInFieldOfView) {
          visibleCubes.Add(obj.cube);
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

    private void SetConstrictedLiveAnimation() {
      // set idle parameters
      Anki.Cozmo.LiveIdleAnimationParameter[] paramNames = {
        Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementDurationMax_ms,
        Anki.Cozmo.LiveIdleAnimationParameter.BodyMovementStraightFraction,
        Anki.Cozmo.LiveIdleAnimationParameter.HeadAngleVariability_deg,
        Anki.Cozmo.LiveIdleAnimationParameter.LiftHeightVariability_mm
      };
      float[] paramValues = {
        3.0f,
        0.2f,
        5.0f,
        0.0f
      };

      _CurrentRobot.SetLiveIdleAnimationParameters(paramNames, paramValues);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      _CurrentRobot.SetLiftHeight(0.0f);
    }

    private bool ShouldEnableSubmitButton(List<LightCube> visibleCubes) {
      bool shouldEnableSubmitButton = false;
      float targetY = _WinningCode.NumCubes * CozmoUtil.kBlockLengthMM * 0.5f + kTargetAreaBuffer;
      Vector3 testCubeCozmoPos;
      bool cubesCloseToCozmo = true;
      foreach (var cube in visibleCubes) {
        testCubeCozmoPos = _CurrentRobot.WorldToCozmo(cube.WorldPosition);
        // TODO: Depend on a BETTER row check, no proximity check?
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
      return shouldEnableSubmitButton;
    }

    private void DriveTowardsCubeMidpoint(List<LightCube> visibleCubes) {
      // Grab the world position midpoint of the cubes
      Vector3 totalDistance = Vector3.zero;
      foreach (var cube in visibleCubes) {
        totalDistance += cube.WorldPosition;
      }
      Vector3 midpointPosition = totalDistance / visibleCubes.Count;

      // Get the vector from Cozmo to that midpoint
      Vector3 facingCubes = midpointPosition - _CurrentRobot.WorldPosition;
      // Convert that vector to radians
      Quaternion facingCubesQuaternion = Quaternion.LookRotation(facingCubes, Vector3.up);

      Vector3 targetPosition = midpointPosition - (facingCubes.normalized * kCubeDistance * (_Game.NumCubesInCode * 0.5f));

      if (Vector3.Distance(targetPosition, _CurrentRobot.WorldPosition) > kDriveToCubeBuffer) {
        _CurrentRobot.GotoPose(targetPosition, facingCubesQuaternion, callback: HandleMoveToCubeCallback);
        _IsDrivingToPose = true;
      }
      else {
        // Check again in a few seconds
        _SecondsSinceLastCubeAdjust = 0f;
      }
    }

    private void DriveToCube(ObservedObject cube) {
      _CurrentRobot.GotoObject(cube, kCubeDistance, callback: HandleMoveToCubeCallback);
      _IsDrivingToPose = true;
    }

    private void HandleMoveToCubeCallback(bool success) {
      _SecondsSinceLastCubeAdjust = 0f;
      _IsDrivingToPose = false;
    }
  }
}
