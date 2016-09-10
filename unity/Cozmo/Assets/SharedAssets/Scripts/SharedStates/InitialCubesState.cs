using Cozmo.UI;
using System.Collections.Generic;

public class InitialCubesState : State {

  private State _NextState;
  protected int _CubesRequired;
  protected ShowCozmoCubeSlide _ShowCozmoCubesSlide;
  protected GameBase _Game;

  private const float _kCubeSqrDistanceWithoutInLift_mm = 360.0f;

  public InitialCubesState(State nextState, int cubesRequired) {
    _NextState = nextState;
    _CubesRequired = cubesRequired;
  }

  public override void Enter() {
    base.Enter();
    SetupRobot();

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

    CheckForNewlySeenCubes();

    Anki.Cozmo.ExternalInterface.PushIdleAnimation pushIdleAnimMsg = new Anki.Cozmo.ExternalInterface.PushIdleAnimation();
    pushIdleAnimMsg.animTrigger = Anki.Cozmo.AnimationTrigger.OnWaitForCubesMinigameSetup;
    RobotEngineManager.Instance.Message.PushIdleAnimation = pushIdleAnimMsg;
    RobotEngineManager.Instance.SendMessage();
  }

  public override void Update() {
    CheckForNewlySeenCubes();
  }

  public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
    // Do nothing
    UpdateUI(0);
    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      lightCube.Value.SetLEDsOff();
    }
  }

  public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
    // Reset cozmo's head
    SetupRobot();
  }

  private void SetupRobot() {
    _CurrentRobot.SetLiftHeight(0f);
    _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
    _CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
  }

  protected virtual void CheckForNewlySeenCubes() {
    bool validCubesChanged = false;
    LightCube cube = null;

    if (null == _CurrentRobot) {
      return;
    }

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      cube = lightCube.Value;
      validCubesChanged |= TryUpdateCubeIdForGame(cube);
    }

    if (validCubesChanged) {
      UpdateUI(_Game.CubeIdsForGame.Count);
    }
  }

  private bool TryUpdateCubeIdForGame(LightCube cube) {
    bool validCubesChanged = false;
    if (cube.IsInFieldOfView) {
      if (IsReallyCloseToCube(cube) && MockRobotTray.Instance == null) {
        validCubesChanged |= RemoveFromValidCubes(cube);
      }
      else {
        validCubesChanged |= AddToValidCubes(cube);
      }
    }
    else {
      validCubesChanged |= RemoveFromValidCubes(cube);
    }
    return validCubesChanged;
  }

  private bool IsReallyCloseToCube(LightCube cube) {
    return (_CurrentRobot.WorldPosition.xy() - cube.WorldPosition.xy()).sqrMagnitude
    <= (_kCubeSqrDistanceWithoutInLift_mm);
  }

  private bool AddToValidCubes(LightCube cube) {
    bool addCube = false;
    if (!_Game.CubeIdsForGame.Contains(cube.ID)) {
      if (_Game.CubeIdsForGame.Count < _CubesRequired) {
        _Game.CubeIdsForGame.Add(cube.ID);
        addCube = true;
        cube.SetLEDs(CubePalette.Instance.InViewColor.lightColor);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Block_Connect);
      }
      else {
        cube.SetLEDs(CubePalette.Instance.InViewColor.lightColor);
      }
    }
    return addCube;
  }

  private bool RemoveFromValidCubes(LightCube cube) {
    bool removedCube = false;
    cube.SetLEDs(CubePalette.Instance.OutOfViewColor.lightColor);
    if (_Game.CubeIdsForGame.Contains(cube.ID)) {
      _Game.CubeIdsForGame.Remove(cube.ID);
      removedCube = true;
    }
    return removedCube;
  }

  protected virtual void UpdateUI(int numValidCubes) {
    _ShowCozmoCubesSlide.LightUpCubes(numValidCubes);

    if (numValidCubes >= _CubesRequired) {
      _Game.SharedMinigameView.SetContinueButtonSupplementText(GetCubesReadyText(_CubesRequired), Cozmo.UI.UIColorPalette.EnergyTextColor);

      _Game.SharedMinigameView.EnableContinueButton(true);
    }
    else {
      _Game.SharedMinigameView.SetContinueButtonSupplementText(GetWaitingForCubesText(_CubesRequired), Cozmo.UI.UIColorPalette.NeutralTextColor);

      _Game.SharedMinigameView.EnableContinueButton(false);
    }
  }

  protected virtual string GetCubesReadyText(int numCubes) {
    string cubesReadyKey = (numCubes > 1) ? LocalizationKeys.kMinigameLabelCubesReadyPlural : LocalizationKeys.kMinigameLabelCubesReadySingular;
    return Localization.GetWithArgs(cubesReadyKey, numCubes);
  }

  protected virtual string GetWaitingForCubesText(int numCubes) {
    string waitingForCubesKey = (numCubes > 1) ? LocalizationKeys.kMinigameLabelWaitingForCubesPlural : LocalizationKeys.kMinigameLabelWaitingForCubesSingular;
    return Localization.Get(waitingForCubesKey);
  }

  public override void Exit() {
    base.Exit();

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      if (!_Game.CubeIdsForGame.Contains(lightCube.Key)) {
        lightCube.Value.SetLEDsOff();
      }
    }

    _Game.SharedMinigameView.HideGameStateSlide();

    RobotEngineManager.Instance.Message.PopIdleAnimation = new Anki.Cozmo.ExternalInterface.PopIdleAnimation();
    RobotEngineManager.Instance.SendMessage();
  }

  protected virtual void HandleContinueButtonClicked() {
    _StateMachine.SetNextState(_NextState);
  }
}
