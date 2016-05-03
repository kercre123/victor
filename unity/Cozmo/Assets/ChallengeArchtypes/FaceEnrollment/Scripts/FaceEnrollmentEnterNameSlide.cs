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
  }

  private void HandleSubmitNameButton() {
    if (OnNameEntered != null) {
      OnNameEntered(_NameInputField.text);
    }
  }
}
