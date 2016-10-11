using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using System;
using Cozmo.MinigameWidgets;

public class DifficultySelectButtonPanel : MonoBehaviour {

  public delegate void DifficultySelectedHandler(float buttonXWorldPosition, bool isUnlocked, DifficultySelectOptionData data);

  public event DifficultySelectedHandler OnDifficultySelected;

  [SerializeField]
  private DifficultySelectOption _OptionPrefab;

  [SerializeField]
  private RectTransform _OptionTray;

  private int _SelectedDifficultyIndex;

  private DifficultySelectOptionData _SelectedDifficulty;

  private readonly List<DifficultySelectOption> _SpawnedOptions = new List<DifficultySelectOption>();

  private int _HighestDifficultyAvailable;

  public DifficultySelectOptionData SelectedDifficulty {
    get {
      return _SelectedDifficulty;
    }
  }

  private void ClearSpawnedOptions() {
    for (int i = 0; i < _SpawnedOptions.Count; i++) {
      Destroy(_SpawnedOptions[i].gameObject);
    }
    _SpawnedOptions.Clear();
  }

  private void HandleDifficultySelected(DifficultySelectOption selectedDifficultyOption) {
    int index = -1;
    for (int i = 0; i < _SpawnedOptions.Count; i++) {
      if (_SpawnedOptions[i] == selectedDifficultyOption) {
        index = i;
      }
    }
    SelectOption(index);
  }


  public void Initialize(List<DifficultySelectOptionData> options, int highestDifficultyAvailable) {
    _HighestDifficultyAvailable = highestDifficultyAvailable;
    for (int i = 0; i < options.Count; i++) {
      var obj = UIManager.CreateUIElement(_OptionPrefab, _OptionTray);

      var option = obj.GetComponent<DifficultySelectOption>();
      option.Initialize(options[i], enabled: options[i].DifficultyId <= _HighestDifficultyAvailable);
      option.OnSelect += HandleDifficultySelected;
      _SpawnedOptions.Add(option);
    }

    _SelectedDifficultyIndex = -1;

    SelectOption(highestDifficultyAvailable);

  }

  private void SelectOption(int index) {
    if (index != _SelectedDifficultyIndex) {
      _SelectedDifficultyIndex = index;
      for (int i = 0; i < _SpawnedOptions.Count; i++) {
        _SpawnedOptions[i].IsSelected = (i == _SelectedDifficultyIndex);
      }

      DifficultySelectOption selectedButton = _SpawnedOptions[index];
      _SelectedDifficulty = selectedButton.Data;

      if (OnDifficultySelected != null) {
        OnDifficultySelected(
          selectedButton.transform.position.x,
          isUnlocked: _SelectedDifficulty.DifficultyId <= _HighestDifficultyAvailable,
          data: _SelectedDifficulty);
      }
    }
  }

  public float GetCurrentlySelectedXWorldPosition() {
    return _SpawnedOptions[_SelectedDifficultyIndex].transform.position.x;
  }
}
