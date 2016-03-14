using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InitialCubesState : State {

  private State _NextState;
  private int _CubesRequired;
  private ShowCozmoCubeSlide _ShowCozmoCubesSlide;
  private int _NumValidCubes;
  private GameBase _Game;

  private const float kCubeTimeoutSeconds = 0.15f;
  private List<int> _ValidCubeIds;
  private Dictionary <int, float> _CubeIdToTimeout;

  public InitialCubesState(State nextState, int cubesRequired) {
    _NextState = nextState;
    _CubesRequired = cubesRequired;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.SetLiftHeight(0f);
    _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
    _CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

    _Game = _StateMachine.GetGame();
    _ShowCozmoCubesSlide = _Game.SharedMinigameView.ShowCozmoCubesSlide(_CubesRequired);
    _Game.SharedMinigameView.ShowContinueButtonOnShelf(HandleContinueButtonClicked,
      Localization.Get(LocalizationKeys.kButtonContinue), 
      Localization.GetWithArgs(LocalizationKeys.kMinigameLabelCubesFound, 0),
      Cozmo.UI.UIColorPalette.NeutralTextColor());
    _Game.SharedMinigameView.EnableContinueButton(false);
    _Game.CubesForGame = new List<LightCube>();
    _CubeIdToTimeout = new Dictionary<int, float>();
    _ValidCubeIds = new List<int>();


    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      lightCube.Value.SetLEDs(Cozmo.CubePalette.OutOfViewColor.lightColor);
    }
  }

  public override void Update() {
    base.Update();
    RemoveTimedOutCubes();
    CheckForNewlySeenCubes();
    if (_ValidCubeIds.Count != _NumValidCubes) {
      _NumValidCubes = _ValidCubeIds.Count;
      UpdateUI();
    }
  }

  private void RemoveTimedOutCubes() {
    List<int> keysToRemove = new List<int>();
    foreach (var key in _ValidCubeIds) {
      if (_CubeIdToTimeout.ContainsKey(key) && _CubeIdToTimeout[key] != -1) {
        _CubeIdToTimeout[key] += Time.deltaTime;
        if (_CubeIdToTimeout[key] >= kCubeTimeoutSeconds) {
          keysToRemove.Add(key);
          _CurrentRobot.LightCubes[key].SetLEDs(Cozmo.CubePalette.OutOfViewColor.lightColor);
          _CubeIdToTimeout[key] = -1;
        }
      }
    }
    foreach (var toRemove in keysToRemove) {
      _ValidCubeIds.Remove(toRemove);
    }
  }

  private void CheckForNewlySeenCubes() {
    LightCube cube = null;
    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      cube = lightCube.Value;
      if (cube.MarkersVisible) {
        if (!_ValidCubeIds.Contains(cube.ID)) {
          if (_ValidCubeIds.Count < _CubesRequired) {
            _ValidCubeIds.Add(cube.ID);
            cube.SetLEDs(Cozmo.CubePalette.InViewColor.lightColor);

            // TODO Put in correct sound
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameStart);
          }
          else {
            cube.SetLEDs(Cozmo.CubePalette.OutOfViewColor.lightColor);
          }
        }
        else if (_CubeIdToTimeout.ContainsKey(cube.ID)) {
          _CubeIdToTimeout[cube.ID] = -1;
        }
      }
      else {
        if (_ValidCubeIds.Contains(cube.ID)) {
          if (!_CubeIdToTimeout.ContainsKey(cube.ID)) {
            _CubeIdToTimeout.Add(cube.ID, 0f);
          }
          else if (_CubeIdToTimeout[cube.ID] == -1) {
            _CubeIdToTimeout[cube.ID] = 0f;
          }
        }
      }
    }

  }

  private void UpdateUI() {
    _ShowCozmoCubesSlide.LightUpCubes(_NumValidCubes);

    if (_NumValidCubes >= _CubesRequired) {
      _Game.SharedMinigameView.SetContinueButtonShelfText(Localization.GetWithArgs(LocalizationKeys.kMinigameLabelCubesReady,
        _CubesRequired), Cozmo.UI.UIColorPalette.CompleteTextColor());

      _Game.SharedMinigameView.EnableContinueButton(true);
    }
    else {
      _Game.SharedMinigameView.SetContinueButtonShelfText(Localization.GetWithArgs(LocalizationKeys.kMinigameLabelCubesFound,
        _NumValidCubes), Cozmo.UI.UIColorPalette.NeutralTextColor());

      _Game.SharedMinigameView.EnableContinueButton(false);
    }

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      if (_ValidCubeIds.Contains(lightCube.Value.ID)) {
        lightCube.Value.SetLEDs(Cozmo.CubePalette.InViewColor.lightColor);
      }
      else {
        lightCube.Value.SetLEDs(Cozmo.CubePalette.OutOfViewColor.lightColor);
      }
    }
  }

  public override void Exit() {
    base.Exit();

    foreach (int id in _ValidCubeIds) {
      _Game.CubesForGame.Add(_CurrentRobot.LightCubes[id]);
    }

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      if (!_Game.CubesForGame.Contains(lightCube.Value)) {
        lightCube.Value.TurnLEDsOff();
      }
    }

    _Game.SharedMinigameView.HideGameStateSlide();
  }

  private void HandleContinueButtonClicked() {
    // TODO: Check if the game has been run before; if so skip the HowToPlayState
    _StateMachine.SetNextState(_NextState);
  }
}
