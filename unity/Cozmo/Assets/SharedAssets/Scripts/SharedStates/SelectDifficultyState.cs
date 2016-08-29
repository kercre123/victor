using System;
using System.Collections.Generic;

public class SelectDifficultyState : State {

  private GameBase _Game;

  private int _HighestLevelCompleted;
  private List<DifficultySelectOptionData> _DifficultyOptions;
  private State _NextState;
  private DifficultySelectButtonPanel _DifficultySelectButtonPanel;
  private DifficultySelectOptionData _SelectedDifficultyData;

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
    if (isUnlocked) {
      _Game.SharedMinigameView.HideLockedBackground();
      _Game.SharedMinigameView.ShowMiddleBackground();
      _SelectedDifficultyData.LoadAnimationPrefabData((UnityEngine.GameObject animationPrefab) => {
        _Game.SharedMinigameView.ShowWideAnimationSlide(_SelectedDifficultyData.DifficultyDescription.Key, data.DifficultyName.Key + "_description",
                                                        animationPrefab, null, LocalizationKeys.kMinigameTextHowToPlayHeader);
      });
    }
    else {
      _Game.SharedMinigameView.ShowWideSlideWithText(data.LockedDifficultyDescription.Key, null);
      _Game.SharedMinigameView.HideMiddleBackground();
      _Game.SharedMinigameView.ShowLockedBackground();
    }
  }

  private void HandleContinueButtonClicked() {
    _Game.CurrentDifficulty = _SelectedDifficultyData.DifficultyId;

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

