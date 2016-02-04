using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InitialCubesState : State {

  private State _NextState;
  private int _CubesRequired;
  private ShowCozmoCubeSlide _ShowCozmoCubesSlide;
  private int _NumValidCubes;
  private GameBase _Game;

  public delegate void CubeStateDone();

  CubeStateDone _CubeStateDone = null;

  public InitialCubesState(State nextState, int cubesRequired, CubeStateDone cubeStateDone = null) {
    _NextState = nextState;
    _CubesRequired = cubesRequired;
    _CubeStateDone = cubeStateDone;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.SetHeadAngle(0f);
    _CurrentRobot.SetLiftHeight(0f);

    _Game = _StateMachine.GetGame();
    _ShowCozmoCubesSlide = _Game.ShowShowCozmoCubesSlide(_CubesRequired);
    _Game.ShowContinueButtonShelf();
    _Game.SetContinueButtonShelfText(string.Format(Localization.GetCultureInfo(),
      Localization.Get(LocalizationKeys.kMinigameLabelShowCubes),
      _CubesRequired,
      0));
    _Game.SetContinueButtonText(Localization.Get(LocalizationKeys.kButtonContinue));
    _Game.SetContinueButtonListener(HandleContinueButtonClicked);
    _Game.EnableContinueButton(false);
  }

  public override void Update() {
    base.Update();

    int numValidCubes = 0;
    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {

      bool isValidCube = lightCube.Value.MarkersVisible;

      if (isValidCube && numValidCubes < _CubesRequired) { 
        lightCube.Value.SetLEDs(Color.white);
        numValidCubes++;
      }
      else {
        lightCube.Value.TurnLEDsOff();
      }
    }

    if (numValidCubes != _NumValidCubes) {
      _NumValidCubes = numValidCubes;
      _ShowCozmoCubesSlide.LightUpCubes(_NumValidCubes);

      if (_NumValidCubes >= _CubesRequired) {
        _Game.SetContinueButtonShelfText(string.Format(Localization.GetCultureInfo(),
          Localization.Get(LocalizationKeys.kMinigameLabelCubesReady),
          _NumValidCubes));

        _Game.EnableContinueButton(true);
      }
      else {
        _Game.SetContinueButtonShelfText(string.Format(Localization.GetCultureInfo(),
          Localization.Get(LocalizationKeys.kMinigameLabelShowCubes),
          _CubesRequired,
          _NumValidCubes));

        _Game.EnableContinueButton(false);
      }
    }
  }

  public override void Exit() {
    base.Exit();

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      lightCube.Value.TurnLEDsOff();
    }

    if (_CubeStateDone != null) {
      _CubeStateDone.Invoke();
    }
    _Game.HideGameStateSlide();
  }

  private void HandleContinueButtonClicked() {
    // TODO: Check if the game has been run before; if so skip the HowToPlayState
    _StateMachine.SetNextState(_NextState);
  }
}
