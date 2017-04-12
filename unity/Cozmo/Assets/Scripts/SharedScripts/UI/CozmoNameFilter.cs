using UnityEngine;
using System.Collections;

public class CozmoNameFilter : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  private void Awake() {
    _NameInputField.onValidateInput += ValidateNameField;
  }

  private char ValidateNameField(string input, int charIndex, char charToValidate) {
    // Allows for japanese, russian, greek etc. But not emojis, punctuation and numbers
    // Flite will just not pronounce characters it can't recognize.
    return char.IsLetter(charToValidate) ? charToValidate : '\0';
  }
}
