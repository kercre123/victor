using UnityEngine;
using System.Collections;
using Anki.UI;
using System;

[System.Serializable]
public class DifficultySelectOptionData {
  public int DifficultyId;
  public LocalizedString DifficultyName;
  public LocalizedString DifficultyDescription;
  public GameObject AnimationPrefab;
  public LocalizedString LockedDifficultyDescription;
}

public class DifficultySelectOption : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoToggleButton _Button;

  [SerializeField]
  private AnkiTextLabel _Label;

  [SerializeField]
  private GameObject _DisabledIcon;


  private DifficultySelectOptionData _Data;

  public DifficultySelectOptionData Data {
    get {
      return _Data;
    }
  }

  public event Action<DifficultySelectOption> OnSelect;

  private bool _IsSelected;
  private bool _IsLocked;

  public bool IsSelected {
    get {
      return _IsSelected;
    }
    set {
      bool wasSelected = _IsSelected;
      _IsSelected = value;
      if (_IsSelected && !wasSelected) {
        if (!_IsLocked) {
          _Button.ShowPressedStateOnRelease = true;
        }
        if (OnSelect != null) {
          OnSelect(this);
        }
      }
      else if (!_IsSelected) {
        _Button.ShowPressedStateOnRelease = false;
      }
    }
  }

  private void Awake() {
    IsSelected = false;
  }

  private void HandleClick() {
    IsSelected = true;
  }

  public void Initialize(DifficultySelectOptionData data, bool enabled) {
    _Data = data;
    _Label.text = _Data.DifficultyName;
    _IsLocked = !enabled;
    _Button.Initialize(HandleClick, "select_" + _Data.DifficultyName, "difficulty_select_slide");
    _Button.ShowDisabledStateWhenInteractable = _IsLocked;
    _Button.gameObject.name = _Data.DifficultyName + "_Button";
    // TODO Change color based on activity vs game
    _Button.PressedTintColor = Cozmo.UI.UIColorPalette.GameToggleColor;

    _DisabledIcon.gameObject.SetActive(_IsLocked);
  }

}
