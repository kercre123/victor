using System;
using UnityEngine;
using System.Collections.Generic;

namespace MemoryMatch {
  public class CozmoMoveCloserToCubesState : State {
    private const float _kCubeIntroBlinkTimes = 0.5f;

    private State _NextState;
    private MemoryMatchGame _GameInstance;

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
      _GameInstance = _StateMachine.GetGame() as MemoryMatchGame;

      _CozmoInPosition = false;
      _FlashingIndex = _WantsCubeBlink ? 0 : _GameInstance.CubeIdsForGame.Count;
      _EndFlashTime = 0;

      // Wait until we get to goal, shouldn't continue
      _GameInstance.InitColorsAndSounds();
      _GameInstance.SharedMinigameView.HideMiddleBackground();
      _GameInstance.SharedMinigameView.HideShelf();
      _GameInstance.SharedMinigameView.HideInstructionsVideoButton();
      if (_ShowLabel) {
        _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kMinigameTextWaitForCozmo);
      }
      PlayerInfo cozmoPlayer = _GameInstance.GetFirstPlayerByType(PlayerType.Cozmo);
      if (cozmoPlayer != null) {
        cozmoPlayer.SetGoal(new MemoryMatchPlayerGoalCozmoPositionReset(_GameInstance, HandleGotoRotationComplete,
                                                                        _DistanceThreshold, _AngleTol_Deg));
      }
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

    private void HandleGotoRotationComplete(PlayerInfo playerBase, int completedCode) {
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

