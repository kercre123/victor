using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InitialCubesState : State {

  private State _NextState;
  private int _CubesRequired;
  private ShowCozmoCubeSlide _ShowCozmoCubesSlide;
  private int _NumValidCubes;
  private GameBase _Game;

  public InitialCubesState(State nextState, int cubesRequired) {
    _NextState = nextState;
    _CubesRequired = cubesRequired;
  }

  public override void Enter() {
    base.Enter();
    _CurrentRobot.SetLiftHeight(0f);
    _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

    _Game = _StateMachine.GetGame();
    _ShowCozmoCubesSlide = _Game.SharedMinigameView.ShowCozmoCubesSlide(_CubesRequired);
    _Game.SharedMinigameView.ShowContinueButtonOnShelf(HandleContinueButtonClicked,
      Localization.Get(LocalizationKeys.kButtonContinue), 
      Localization.GetWithArgs(LocalizationKeys.kMinigameLabelCubesFound, 0),
      Cozmo.UI.UIColorPalette.NeutralTextColor());
    _Game.SharedMinigameView.EnableContinueButton(false);
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
        _Game.SharedMinigameView.SetContinueButtonShelfText(Localization.GetWithArgs(LocalizationKeys.kMinigameLabelCubesReady,
          _NumValidCubes), Cozmo.UI.UIColorPalette.CompleteTextColor());

        _Game.SharedMinigameView.EnableContinueButton(true);
      }
      else {
        _Game.SharedMinigameView.SetContinueButtonShelfText(Localization.GetWithArgs(LocalizationKeys.kMinigameLabelCubesFound,
          _CubesRequired,
          _NumValidCubes), Cozmo.UI.UIColorPalette.NeutralTextColor());

        _Game.SharedMinigameView.EnableContinueButton(false);
      }
    }
  }

  public override void Exit() {
    base.Exit();

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      lightCube.Value.TurnLEDsOff();
    }

    _Game.SharedMinigameView.HideGameStateSlide();
  }

  private void HandleContinueButtonClicked() {
    // TODO: Check if the game has been run before; if so skip the HowToPlayState
    _StateMachine.SetNextState(_NextState);
  }
}
