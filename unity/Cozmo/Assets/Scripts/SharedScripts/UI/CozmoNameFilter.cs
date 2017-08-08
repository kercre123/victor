using UnityEngine;
using System.Collections;
using System.Linq;

public class CozmoNameFilter : MonoBehaviour {

  [SerializeField]
  private TMPro.TMP_InputField _NameInputField;

  // because the french like double names
  [SerializeField]
  private char[] _ValidNonLetterCharacters = { '-', ' ' };

  private void Awake() {
    _NameInputField.onValidateInput += ValidateNameField;
  }

  private char ValidateNameField(string input, int charIndex, char charToValidate) {
    // Allows for japanese, russian, greek etc. But not emojis, punctuation and numbers
    // Flite will just not pronounce characters it can't recognize.
    bool isAllowed = char.IsLetter(charToValidate) || _ValidNonLetterCharacters.Contains(charToValidate);
    return isAllowed ? charToValidate : '\0';
  }
}
