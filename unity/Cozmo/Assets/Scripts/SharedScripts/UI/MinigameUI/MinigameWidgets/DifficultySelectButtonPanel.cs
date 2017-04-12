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
  private GameObject _DemoSkillBtnPrefab;

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


  public void Initialize(List<DifficultySelectOptionData> options, int highestDifficultyAvailable, int selectedDifficulty) {
    _HighestDifficultyAvailable = highestDifficultyAvailable;
    for (int i = 0; i < options.Count; i++) {
      var obj = UIManager.CreateUIElement(_OptionPrefab, _OptionTray);

      var option = obj.GetComponent<DifficultySelectOption>();
      option.Initialize(options[i], enabled: options[i].DifficultyId <= _HighestDifficultyAvailable);
      option.OnSelect += HandleDifficultySelected;
      _SpawnedOptions.Add(option);
    }

    _SelectedDifficultyIndex = -1;

    SelectOption(selectedDifficulty);

    // Show allow override for skill level
    if (DebugMenuManager.Instance.DemoMode) {
      CreateDemoSkillBtn("Skill: Default", SkillSystem.SkillOverrideLevel.None, 0);
      CreateDemoSkillBtn("Skill: Easy", SkillSystem.SkillOverrideLevel.Min, 600);
      CreateDemoSkillBtn("Skill: Hard", SkillSystem.SkillOverrideLevel.Max, 1200);
    }
  }

  private void CreateDemoSkillBtn(string btnName, SkillSystem.SkillOverrideLevel level, int xOffset = 0) {
    GameObject newButton = UIManager.CreateUIElement(_DemoSkillBtnPrefab, transform.parent.parent.transform);
    newButton.transform.localPosition = new Vector3(xOffset, 350);
    UnityEngine.UI.Button button = newButton.GetComponent<UnityEngine.UI.Button>();
    UnityEngine.UI.Text txt = button.GetComponentInChildren<UnityEngine.UI.Text>();
    if (txt) {
      txt.text = btnName;
    }
    button.onClick.AddListener(() => {
      SkillSystem.Instance.SetSkillOverride(level);
    });
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
