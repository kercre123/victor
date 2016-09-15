using UnityEngine;
using System.Collections;

public class CozmoNameFilter : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  private void Awake() {
    _NameInputField.onValidateInput += ValidateNameField;
  }

  private char ValidateNameField(string input, int charIndex, char charToValidate) {
    if (charToValidate >= 'a' && charToValidate <= 'z' || charToValidate >= 'A' && charToValidate <= 'Z') {
      return charToValidate;
    }
    return '\0';
  }
}
