using System;
using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class CozmoMoveCloserToCubesState : State {
    public const float kTargetDistance = 125f;
    public const float kDistanceThreshold = 20f;
    float kAngleTolerance = 2.5f;
    private const float _kCubeIntroBlinkTimes = 0.5f;

    private State _NextState;
    private SimonGame _GameInstance;
    private Vector2 _TargetPosition;
    private Quaternion _TargetRotation;
    private Vector2 _CubeMidpoint;

    private bool _CozmoInPosition;
    private int _FlashingIndex;
    private float _EndFlashTime;

    public CozmoMoveCloserToCubesState(State nextState) {
      _NextState = nextState;
    }

    public override void Enter() {
      base.Enter();
      _CozmoInPosition = false;
      _FlashingIndex = 0;
      _EndFlashTime = 0;

      List<LightCube> cubesForGame = new List<LightCube>();
      // Wait until we get to goal, shouldn't continue
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.InitColorsAndSounds();
      _GameInstance.SharedMinigameView.EnableContinueButton(false);
      _GameInstance.SharedMinigameView.HideMiddleBackground();

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
      if (_CurrentRobot.WorldPosition.xy().IsNear(targetPosition, kDistanceThreshold)) {
        HandleGotoPoseComplete(true);
      }
      else {
        _CurrentRobot.GotoPose(targetPosition, targetRotation, callback: HandleGotoPoseComplete);
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
      if (_CurrentRobot.Rotation.IsSimilarAngle(targetRotation, kAngleTolerance)) {
        HandleGotoRotationComplete(true);
      }
      else {
        _CurrentRobot.GotoPose(_TargetPosition, targetRotation, callback: HandleGotoRotationComplete);
      }
    }

    private void HandleGotoRotationComplete(bool success) {
      _CozmoInPosition = true;
      GoToNextState();
    }
    private void GoToNextState() {
      if (_CozmoInPosition && _FlashingIndex >= _GameInstance.CubeIdsForGame.Count) {
        _StateMachine.SetNextState(_NextState);
      }
    }

    #region Target Position calculations

    #endregion
  }
}

