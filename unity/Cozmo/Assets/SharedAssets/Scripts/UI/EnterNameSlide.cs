using UnityEngine;
using System.Collections;
using Cozmo.UI;

public class EnterNameSlide : MonoBehaviour {

  public System.Action<string> OnNameEntered;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SubmitName;

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  private void Awake() {
    _SubmitName.Initialize(HandleSubmitNameButton, "enter_name_done", "enter_name_slide");
    _SubmitName.Interactable = false;
  }

  private void Start() {
    if (string.IsNullOrEmpty(_NameInputField.text) == false) {
      _SubmitName.Interactable = true;
    }
  }

  public void SetNameInputField(string existing) {
    if (string.IsNullOrEmpty(existing)) {
      return;
    }
    // there is a bug with unity's InputField for Name types that cuts off the last character of a programmatically
    // set string. This is a workaround.
    // https://fogbugz.unity3d.com/default.asp?824198_olip6sa7g9joavuc
    _NameInputField.characterLimit = _NameInputField.characterLimit + 1;
    _NameInputField.text = existing;
    _NameInputField.characterLimit = _NameInputField.characterLimit - 1;
  }

  public void RegisterInputFocus() {
    _NameInputField.Select();
    _NameInputField.ActivateInputField();
    _NameInputField.onValueChanged.AddListener(HandleInputFieldChange);
  }

  private void HandleInputFieldChange(string input) {
    if (string.IsNullOrEmpty(input)) {
      _SubmitName.Interactable = false;
    }
    else {
      _SubmitName.Interactable = true;
    }
  }

  private void HandleSubmitNameButton() {
    _NameInputField.onValueChanged.RemoveListener(HandleInputFieldChange);
    _SubmitName.Interactable = false;
    if (OnNameEntered != null) {
      OnNameEntered(_NameInputField.text);
    }
  }
}
