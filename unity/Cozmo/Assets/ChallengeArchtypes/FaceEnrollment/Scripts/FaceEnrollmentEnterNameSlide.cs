using UnityEngine;
using System.Collections;
using Cozmo.UI;

public class FaceEnrollmentEnterNameSlide : MonoBehaviour {

  public System.Action<string> OnNameEntered;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SubmitName;

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  private void Start() {
    _SubmitName.onClick.AddListener(HandleSubmitNameButton);
    _SubmitName.Interactable = false;
    _NameInputField.onValueChanged.AddListener(HandleInputFieldDone);
    _NameInputField.ActivateInputField();
  }

  private void HandleInputFieldDone(string input) {
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
