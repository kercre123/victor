using System;
using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class CozmoMoveCloserToCubesState : State {
    public const float kTargetDistance = 125f;
    private const float _kCubeIntroBlinkTimes = 0.5f;

    private State _NextState;
    private SimonGame _GameInstance;
    private Vector2 _TargetPosition;
    private Quaternion _TargetRotation;
    private Vector2 _CubeMidpoint;

    private bool _CozmoInPosition;
    private int _FlashingIndex;
    private float _EndFlashTime;

    private bool _WantsCubeBlink;
    private float _DistanceThreshold;
    private float _AngleTol_Deg;
    private bool _ShowLabel;

    public CozmoMoveCloserToCubesState(State nextState, bool wantsBlink = true, float distanceThreshold = 20f, float angleTol_Deg = 2.5f, bool showLabel = true) {
      _NextState = nextState;
      _WantsCubeBlink = wantsBlink;
      _DistanceThreshold = distanceThreshold;
      _AngleTol_Deg = angleTol_Deg;
      _ShowLabel = showLabel;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;

      _CozmoInPosition = false;
      _FlashingIndex = _WantsCubeBlink ? 0 : _GameInstance.CubeIdsForGame.Count;
      _EndFlashTime = 0;

      List<LightCube> cubesForGame = new List<LightCube>();
      // Wait until we get to goal, shouldn't continue
      _GameInstance.InitColorsAndSounds();
      _GameInstance.SharedMinigameView.HideMiddleBackground();
      _GameInstance.SharedMinigameView.HideShelf();
      _GameInstance.SharedMinigameView.HideInstructionsVideoButton();
      if (_ShowLabel) {
        _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kMinigameTextWaitForCozmo);
      }

      IRobot robot = _GameInstance.CurrentRobot;
      foreach (int id in _GameInstance.CubeIdsForGame) {
        cubesForGame.Add(robot.LightCubes[id]);
      }
      LightCube cubeA, cubeB;
      LightCube.TryFindCubesFurthestApart(cubesForGame, out cubeA, out cubeB);
      _CubeMidpoint = VectorUtil.Midpoint(cubeA.WorldPosition.xy(), cubeB.WorldPosition.xy());
      Vector2 cubeAlignmentVector = cubeA.WorldPosition - cubeB.WorldPosition;
      Vector2 perpendicularToCubes = cubeAlignmentVector.PerpendicularAlignedWith(_CurrentRobot.Forward.xy());

      // Add the vector to the center of the blocks to figure out the target world position
      _TargetPosition = _CubeMidpoint + (-perpendicularToCubes.normalized * kTargetDistance);
      float targetAngle = Mathf.Atan2(perpendicularToCubes.y, perpendicularToCubes.x) * Mathf.Rad2Deg;
      _TargetRotation = Quaternion.Euler(0, 0, targetAngle);

      MoveToTargetLocation(_TargetPosition, _TargetRotation);
    }

    public override void Exit() {
      base.Exit();
      if (_ShowLabel) {
        _GameInstance.SharedMinigameView.InfoTitleText = "";
      }
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
    }

    public override void Update() {
      base.Update();

      if (_EndFlashTime < Time.time && _FlashingIndex < _GameInstance.CubeIdsForGame.Count) {
        _EndFlashTime = Time.time + _kCubeIntroBlinkTimes;
        int id = _GameInstance.CubeIdsForGame[_FlashingIndex];
        _GameInstance.BlinkLight(id, _kCubeIntroBlinkTimes, Color.black, _GameInstance.GetColorForBlock(id));
        ++_FlashingIndex;
        if (_FlashingIndex >= _GameInstance.CubeIdsForGame.Count) {
          GoToNextState();
        }
      }

    }

    private void MoveToTargetLocation(Vector2 targetPosition, Quaternion targetRotation) {
      // Skip moving if we're already close to the target
      if (_CurrentRobot != null) {
        if (_CurrentRobot.WorldPosition.xy().IsNear(targetPosition, _DistanceThreshold)) {
          HandleGotoPoseComplete(true);
        }
        else {
          _CurrentRobot.GotoPose(targetPosition, targetRotation, callback: HandleGotoPoseComplete);
        }
      }
      else {
        HandleGotoRotationComplete(false);
      }
    }

    private void HandleGotoPoseComplete(bool success) {
      if (success) {
        MoveToTargetRotation(_TargetRotation);
      }
      else {
        MoveToTargetLocation(_TargetPosition, _TargetRotation);
      }
    }

    private void MoveToTargetRotation(Quaternion targetRotation) {
      if (_CurrentRobot != null) {
        if (_CurrentRobot.Rotation.IsSimilarAngle(targetRotation, _AngleTol_Deg)) {
          HandleGotoRotationComplete(true);
        }
        else {
          _CurrentRobot.GotoPose(_TargetPosition, targetRotation, callback: HandleGotoRotationComplete);
        }
      }
      else {
        HandleGotoRotationComplete(false);
      }
    }

    private void HandleGotoRotationComplete(bool success) {
      _CozmoInPosition = true;
      GoToNextState();
    }
    private void GoToNextState() {
      if (_CozmoInPosition && _FlashingIndex >= _GameInstance.CubeIdsForGame.Count) {
        // using this state to correct position
        if (_NextState == null) {
          _StateMachine.PopState();
        }
        else {
          _StateMachine.SetNextState(_NextState);
        }
      }
    }

    #region Target Position calculations

    #endregion
  }
}

