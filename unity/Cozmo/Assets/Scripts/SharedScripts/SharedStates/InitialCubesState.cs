using Cozmo.UI;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

public class InitialCubesState : State {

  private State _NextState;
  protected int _CubesRequired;
  protected ShowCozmoCubeSlide _ShowCozmoCubesSlide;
  protected GameBase _Game;
  protected bool _ShowTransparentCube = true;

  private const float _kCubeSqrDistanceWithoutInLift_mm = 360.0f;
  private bool _PushedIdleAnimation = false;

  private bool _IsPlayingReactionAnim = false;

  private const string kInitialCubesStateIdleLock = "initial_cubes_state";

  public InitialCubesState(State nextState, int cubesRequired) {
    _NextState = nextState;
    _CubesRequired = cubesRequired;
  }

  public override void Enter() {
    base.Enter();
    SetupRobot();

    _Game = _StateMachine.GetGame();

    _ShowCozmoCubesSlide = _Game.SharedMinigameView.ShowCozmoCubesSlide(_CubesRequired, _ShowTransparentCube);
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
  }

  public override void Update() {
    CheckForNewlySeenCubes();
  }

  public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    // Do nothing
    UpdateUI(0);
    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      lightCube.Value.SetLEDsOff();
    }
  }

  public override void Resume(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
    // Don't interrupt the process of going to sleep
    if (!Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled) {
      // Reset cozmo's head
      SetupRobot();
    }
  }

  private void SetupRobot() {
    _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.GameSetupGetIn);
    _CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
    PushIdleAnimation();
  }

  protected virtual void CheckForNewlySeenCubes() {
    // The reaction animation has head movement, so don't
    // mark cubes as out of view while the animation is playing
    if (null == _CurrentRobot || _IsPlayingReactionAnim) {
      return;
    }

    bool validCubesChanged = false;
    LightCube cube = null;
    bool wasAdded = false;
    LightCube lastAddedCube = null;

    foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
      cube = lightCube.Value;
      validCubesChanged |= TryUpdateCubeIdForGame(cube, out wasAdded);
      if (wasAdded) {
        // Cache cube so that we can tilt head towards it
        lastAddedCube = cube;
      }
    }

    if (validCubesChanged) {
      UpdateUI(_Game.CubeIdsForGame.Count);
      if (_Game.CubeIdsForGame.Count == 0) {
        PushIdleAnimation();
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.GameSetupGetOut);
      }
      else if (lastAddedCube != null) {
        PopIdleAnimation();
        _IsPlayingReactionAnim = true;

        // Stomp over any get out animation if it's playing
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.GameSetupReaction,
                                           queueActionPosition: Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING);

        // COZMO-7732: Can't rely on the animation to keep the cubes in view, so we need to turn the head towards
        // the newly added cube. maxTurnAngle_rad = 0 means that the robot will not turn
        _CurrentRobot.TurnTowardsObject(lastAddedCube, headTrackWhenDone: false, callback: HandlePlayReactionFinished,
                                        queueActionPosition: Anki.Cozmo.QueueActionPosition.AT_END, maxTurnAngle_rad: 0f);
      }
    }
  }

  private void HandlePlayReactionFinished(bool success) {
    _IsPlayingReactionAnim = false;

    // Possible to play reaction on cubes 1 and 2, but we require 3
    if (_Game.CubeIdsForGame.Count >= _CubesRequired) {
      _Game.SharedMinigameView.SetContinueButtonSupplementText(null, Cozmo.UI.UIColorPalette.NeutralTextColor);
      _Game.SharedMinigameView.EnableContinueButton(true);
    }
  }

  private bool TryUpdateCubeIdForGame(LightCube cube, out bool wasAdded) {
    bool validCubesChanged = false;
    wasAdded = false;
    if (cube.IsInFieldOfView) {
      if (IsReallyCloseToCube(cube) && MockRobotTray.Instance == null) {
        validCubesChanged = RemoveFromValidCubes(cube);
      }
      else {
        if (AddToValidCubes(cube)) {
          validCubesChanged = true;
          wasAdded = true;
        }
      }
    }
    else {
      validCubesChanged = RemoveFromValidCubes(cube);
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
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Block_Connect);
      }
      else {
        // Don't set light state to in-view if they are not needed for the game
        cube.SetLEDs(CubePalette.Instance.OutOfViewColor.lightColor);
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
      _Game.SharedMinigameView.SetContinueButtonSupplementText(GetCubesReadyText(_CubesRequired), Cozmo.UI.UIColorPalette.MinigameTextColor);
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
    if (_CurrentRobot != null) {
      foreach (KeyValuePair<int, LightCube> lightCube in _CurrentRobot.LightCubes) {
        if (!_Game.CubeIdsForGame.Contains(lightCube.Key)) {
          lightCube.Value.SetLEDsOff();
        }
      }

      PopIdleAnimation();
    }

    _Game.SharedMinigameView.HideGameStateSlide();
  }

  protected virtual void HandleContinueButtonClicked() {
    // we should ONLY ever set a new state if we're still the current state
    // tends to be a reoccuring bug worth logging since it's on sharedminigameview button and needs be reset per state.
    if (_StateMachine.GetCurrState() == this) {
      _StateMachine.SetNextState(_NextState);
    }
    else {
      DAS.Error("InitialCubeState.SetNextState.AttemptingToSetStateWhenNotActive", "");
    }
  }
}
