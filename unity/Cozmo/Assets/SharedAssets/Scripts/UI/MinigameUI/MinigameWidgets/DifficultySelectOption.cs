using UnityEngine;
using System.Collections;
using Anki.UI;
using System;

[System.Serializable]
public class DifficultySelectOptionData {  
  public int DifficultyId;
  public string DifficultyName;
  public string DifficultyDescription;
}

public class DifficultySelectOption : MonoBehaviour {

  [SerializeField]
  private AnkiButton _Button;

  [SerializeField]
  private AnkiTextLabel _Label;

  [SerializeField]
  private GameObject _SelectedMark;


  private DifficultySelectOptionData _Data;

  public DifficultySelectOptionData Data {
    get {
      return _Data;
    }
  }

  public event Action<DifficultySelectOption> OnSelect;

  private bool _IsSelected;

  public bool IsSelected {
    get {
      return _IsSelected;
    }
    set {
      bool wasSelected = _IsSelected;
      _IsSelected = value;
      if (_IsSelected && !wasSelected) {
        if (OnSelect != null) {
          OnSelect(this);
        }
      }
      _SelectedMark.gameObject.SetActive(_IsSelected);
    }
  }

  private void Awake() {
    IsSelected = false;
    _Button.onClick.AddListener(HandleClick);
  }

  private void HandleClick() {
    IsSelected = true;
  }

  public void Initialize(DifficultySelectOptionData data, bool enabled) {
    _Data = data;
    _Label.text = _Data.DifficultyName;
    _Button.Interactable = enabled;
  }

}
