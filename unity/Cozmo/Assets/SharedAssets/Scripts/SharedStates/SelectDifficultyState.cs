using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

public class SelectDifficultyState : State {

  private GameBase _Game;

  private int _HighestLevelCompleted;
  private List<DifficultySelectOptionData> _DifficultyOptions;
  private State _NextState;
  private DifficultySelectButtonPanel _DifficultySelectButtonPanel;
  private DifficultySelectOptionData _SelectedDifficultyData;
  private bool _DifficultySelectLockState = false;

  public SelectDifficultyState(State nextState, List<DifficultySelectOptionData> difficultyOptions, int highestLevelCompleted) {
    _NextState = nextState;
    _DifficultyOptions = difficultyOptions;
    _HighestLevelCompleted = highestLevelCompleted;
  }

  public override void Enter() {
    base.Enter();
    _Game = _StateMachine.GetGame();
    _Game.SharedMinigameView.ShowContinueButtonOffset(HandleContinueButtonClicked,
      Localization.Get(LocalizationKeys.kButtonContinue), string.Empty, UnityEngine.Color.clear,
      "selected_difficulty_continue_button");
    _Game.SharedMinigameView.ShelfWidget.GrowShelfBackground();

    _Game.SharedMinigameView.ShowTallShelf(true);
    _Game.SharedMinigameView.EnableContinueButton(false);
    _DifficultySelectButtonPanel = _Game.SharedMinigameView.ShowDifficultySelectButtons(_DifficultyOptions,
      _HighestLevelCompleted, HandleInitialDifficultySelected);
  }

  public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
    // Do nothing
  }

  public override void Resume(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
    // Do nothing
  }

  public override void Exit() {
    _Game.SharedMinigameView.ShowTallShelf(false);
    _DifficultySelectButtonPanel.OnDifficultySelected -= HandleDifficultySelected;
  }

  private void HandleInitialDifficultySelected() {
    _DifficultySelectButtonPanel.OnDifficultySelected += HandleDifficultySelected;
    HandleDifficultySelected(_DifficultySelectButtonPanel.GetCurrentlySelectedXWorldPosition(), true,
      _DifficultySelectButtonPanel.SelectedDifficulty);
  }

  private void HandleDifficultySelected(float buttonXWorldPosition, bool isUnlocked, DifficultySelectOptionData data) {
    _SelectedDifficultyData = data;
    _Game.SharedMinigameView.ShelfWidget.MoveCarat(buttonXWorldPosition);
    _Game.SharedMinigameView.EnableContinueButton(isUnlocked);

    _DifficultySelectLockState = isUnlocked;

    if (isUnlocked) {
      _Game.SharedMinigameView.HideLockedBackground();
      _Game.SharedMinigameView.ShowMiddleBackground();

      DifficultySelectOptionData unlockedSelection = data;
      _SelectedDifficultyData.LoadAnimationPrefabData((UnityEngine.GameObject animationPrefab) => {
        // guards against async issue where two buttons were pressed right after each other.
        if (_DifficultySelectLockState) {
          if (unlockedSelection != null) {
            _Game.SharedMinigameView.ShowWideAnimationSlide(unlockedSelection.DifficultyDescription.Key, unlockedSelection.DifficultyName.Key + "_description",
                                                  animationPrefab, null, LocalizationKeys.kMinigameTextHowToPlayHeader);
          }
          else {
            DAS.Info("SelectDifficultyState.HandleDifficultySelected", "unlockedSelection is null");
          }
        }
      });
    }
    else {
      _Game.SharedMinigameView.ShowWideSlideWithText(data.LockedDifficultyDescription.Key, null);
      _Game.SharedMinigameView.HideMiddleBackground();
      _Game.SharedMinigameView.ShowLockedBackground();
    }
  }

  private void HandleContinueButtonClicked() {
    if (_SelectedDifficultyData == null) {
      DAS.Warn("UnityEngine.SelectDifficultyState.HandleContinueButtonClicked", "_SelectedDifficultyData is null");
      return;
    }
    _Game.CurrentDifficulty = _SelectedDifficultyData.DifficultyId;
    DAS.Event("game.difficulty", _SelectedDifficultyData.DifficultyId.ToString());

    // Don't tween transitions in Exit because that will cause errors in DoTween if exiting 
    // the state machine is through the quit button
    _Game.SharedMinigameView.ShelfWidget.HideCaratOffscreenLeft();
    _Game.SharedMinigameView.ShelfWidget.ShrinkShelfBackground();
    _Game.SharedMinigameView.HideDifficultySelectButtonPanel();
    _Game.SharedMinigameView.HideContinueButton();
    _Game.SharedMinigameView.HideGameStateSlide();

    _SelectedDifficultyData.LoadAnimationPrefabData((UnityEngine.GameObject animationPrefab) => {

    });

    _StateMachine.SetNextState(_NextState);
  }
}

