using UnityEngine;
using System.Collections;
using Cozmo.UI;

public class FaceEnrollmentEnterNameSlide : MonoBehaviour {

  public System.Action<string> OnNameEntered;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SubmitName;

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  private void Awake() {
    _SubmitName.Initialize(HandleSubmitNameButton, "face_enrollment_name_enter_done", "face_enrollment_name_slide");
    _SubmitName.Interactable = false;
  }

  public void RegisterInputFocus() {
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
    if (OnNameEntered != null) {
      OnNameEntered(_NameInputField.text);
    }
  }
}
