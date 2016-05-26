using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InitialCubesState : State {

  private State _NextState;
  private int _CubesRequired;
  private ShowCozmoCubeSlide _ShowCozmoCubesSlide;
  private int _NumValidCubes;
  private GameBase _Game;

  // TODO: Use RobotProcessedImage to count how many ticks / vision frames a cube
  // is not visible instead of using a time-based timeout
  private const float kCubeTimeoutSeconds = 0.4f;
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
    _Game.SharedMinigameView.ShowContinueButtonOffset(HandleContinueButtonClicked,
      Localization.Get(LocalizationKeys.kButtonContinue), 
      GetWaitingForCubesText(_CubesRequired),
      Cozmo.UI.UIColorPalette.NeutralTextColor,
      "cubes_are_ready_continue_button");
    _Game.SharedMinigameView.EnableContinueButton(false);
    _Game.SharedMinigameView.ShowShelf();
    _Game.SharedMinigameView.ShowMiddleBackground();

    _Game.CubeIdsForGame = new List<int>();
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
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedBlockConnect);
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
      _Game.SharedMinigameView.SetContinueButtonSupplementText(GetCubesReadyText(_CubesRequired), Cozmo.UI.UIColorPalette.CompleteTextColor);

      _Game.SharedMinigameView.EnableContinueButton(true);
    }
    else {
      _Game.SharedMinigameView.SetContinueButtonSupplementText(GetWaitingForCubesText(_CubesRequired), Cozmo.UI.UIColorPalette.NeutralTextColor);

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

  private string GetCubesReadyText(int numCubes) {
    string cubesReadyKey = (numCubes > 1) ? LocalizationKeys.kMinigameLabelCubesReadyPlural : LocalizationKeys.kMinigameLabelCubesReadySingular;
    return Localization.GetWithArgs(cubesReadyKey, numCubes);
  }

  private string GetWaitingForCubesText(int numCubes) {
    string waitingForCubesKey = (numCubes > 1) ? LocalizationKeys.kMinigameLabelWaitingForCubesPlural : LocalizationKeys.kMinigameLabelWaitingForCubesSingular;
    return Localization.Get(waitingForCubesKey);
  }

  public override void Exit() {
    base.Exit();

    foreach (int id in _ValidCubeIds) {
      _Game.CubeIdsForGame.Add(id);
    }

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      if (!_Game.CubeIdsForGame.Contains(lightCube.Key)) {
        lightCube.Value.SetLEDsOff();
      }
    }

    _Game.SharedMinigameView.HideGameStateSlide();
  }

  private void HandleContinueButtonClicked() {
    // TODO: Check if the game has been run before; if so skip the HowToPlayState
    _StateMachine.SetNextState(_NextState);
  }
}
