using UnityEngine;

public class CozmoNameFilter : MonoBehaviour {

  [SerializeField]
  private TMPro.TMP_InputField _NameInputField;

  private void Awake() {
    _NameInputField.onValidateInput += ValidateNameField;
  }

  private void OnDestroy() {
    _NameInputField.onValidateInput -= ValidateNameField;
  }

  private char ValidateNameField(string input, int charIndex, char charToValidate) {
    return Cozmo.UI.CozmoInputFilter.IsValidInput(charToValidate, allowPunctuation: false, allowDigits: false) ? charToValidate : '\0';
  }
}
