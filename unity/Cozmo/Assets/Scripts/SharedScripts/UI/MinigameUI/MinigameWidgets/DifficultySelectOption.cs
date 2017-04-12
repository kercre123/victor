using UnityEngine;
using Anki.UI;
using System;
using Anki.Assets;

[Serializable]
public class DifficultySelectOptionData {
  public int DifficultyId;
  public LocalizedString DifficultyName;
  public LocalizedString DifficultyDescription;
  public LocalizedString LockedDifficultyDescription;

  [SerializeField]
  private GameObjectDataLink _AnimationPrefabData;

  public void LoadAnimationPrefabData(Action<GameObject> dataLoadedCallback) {
    _AnimationPrefabData.LoadAssetData(dataLoadedCallback);
  }
}

public class DifficultySelectOption : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoRadioButton _Button;

  [SerializeField]
  private AnkiTextLegacy _Label;

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
        _Button.ShowPressedStateOnRelease = true;
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
    _IsLocked = !enabled;
    if (_IsLocked) {
      _Label.gameObject.SetActive(false);
    }
    else {
      _Label.text = _Data.DifficultyName;
    }
    _Button.Initialize(HandleClick, "select_" + _Data.DifficultyName, "difficulty_select_slide");
    _Button.gameObject.name = _Data.DifficultyName + "_Button";

    _DisabledIcon.gameObject.SetActive(_IsLocked);
  }

}
