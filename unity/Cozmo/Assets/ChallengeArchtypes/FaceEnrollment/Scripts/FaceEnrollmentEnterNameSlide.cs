using UnityEngine;
using System.Collections;
using Cozmo.UI;

public class FaceEnrollmentEnterNameSlide : MonoBehaviour {

  public System.Action<string> OnNameEntered;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SubmitName;

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _NameInputPlaceholder;

  private void Awake() {
    _SubmitName.Initialize(HandleSubmitNameButton, "face_enrollment_name_enter_done", "face_enrollment_name_slide");
    _SubmitName.Interactable = false;
    _NameInputField.onValidateInput += ValidateNameField;
  }

  private void Start() {
    if (string.IsNullOrEmpty(_NameInputField.text) == false) {
      _SubmitName.Interactable = true;
      _NameInputPlaceholder.enabled = false;
    }
  }

  public void SetNameInputField(string existing) {
    // there is a bug with unity's InputField for Name types that cuts off the last character of a programmatically
    // set string. This is a workaround.
    // https://fogbugz.unity3d.com/default.asp?824198_olip6sa7g9joavuc
    _NameInputField.characterLimit = _NameInputField.characterLimit + 1;
    _NameInputField.text = existing;
    _NameInputField.characterLimit = _NameInputField.characterLimit - 1;
  }

  private char ValidateNameField(string input, int charIndex, char charToValidate) {
    if (charToValidate >= 'a' && charToValidate <= 'z' || charToValidate >= 'A' && charToValidate <= 'Z') {
      return charToValidate;
    }
    return '\0';
  }

  public void RegisterInputFocus() {
    _NameInputField.Select();
    _NameInputField.ActivateInputField();
    _NameInputField.onValueChanged.AddListener(HandleInputFieldChange);
  }

  private void HandleInputFieldChange(string input) {
    if (string.IsNullOrEmpty(input)) {
      _SubmitName.Interactable = false;
      _NameInputPlaceholder.enabled = true;
    }
    else {
      _SubmitName.Interactable = true;
      _NameInputPlaceholder.enabled = false;
    }
  }

  private void HandleSubmitNameButton() {
    if (OnNameEntered != null) {
      OnNameEntered(_NameInputField.text);
    }
  }
}
