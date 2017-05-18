using UnityEngine;
using System.Collections;

// useful for setting long bodies of text (like privacy policy).
// this is necessary because unity's text rendering is garbage and
// have a small cap (~16k characters)
public class AnkiInfiniteScrollView : MonoBehaviour {

  [SerializeField]
  private RectTransform _ContentContainer;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _AnkiInfiniteScrollTextCellPrefab;

  private string[] _StringsToDisplay;
  private Coroutine _CurrentRoutine;

  private const int _kStringsToProcessPerFrame = 10;
  private const int _kCharactersPerString = 16000; // See http://forum.unity3d.com/threads/apparent-string-length-limit.30874/

  public void SetString(string longString) {
    ClearContainer();

    // Equivalent to Math.Ceiling(longString.Length / _kCharactersPerString) but without the int <-> double conversions
    int numberOfStrings = (longString.Length + _kCharactersPerString - 1) / _kCharactersPerString; 
    _StringsToDisplay = new string[numberOfStrings];

    if (_CurrentRoutine != null) {
      StopCoroutine(_CurrentRoutine);
    }
    _CurrentRoutine = StartCoroutine(CreateTextLabels(longString));
  }

  private IEnumerator CreateTextLabels(string longString) {
    int currentSubString = 0;
    int subStringStartIndex = 0;

    while (currentSubString < _StringsToDisplay.Length) {
      int targetIndex = System.Math.Min(currentSubString + _kStringsToProcessPerFrame, _StringsToDisplay.Length);

      while (currentSubString < targetIndex) {
        Anki.UI.AnkiTextLegacy scrollTextCell = GameObject.Instantiate(_AnkiInfiniteScrollTextCellPrefab.gameObject).GetComponent<Anki.UI.AnkiTextLegacy>();
        scrollTextCell.transform.SetParent(_ContentContainer, false);

        // Calculate start and length of the substring
        int remainingCharacters = longString.Length - subStringStartIndex;
        if (remainingCharacters <= _kCharactersPerString) {
          // Last page, copy the rest of the text
          scrollTextCell.text = longString.Substring(subStringStartIndex, remainingCharacters);
        }
        else {
          // Grab as much text as we can
          int subStringLength = _kCharactersPerString;

          // Back up until we find whitespace, to avoid breaking in the middle of a word
          while (subStringLength > 0) {
            char c = longString[subStringStartIndex + subStringLength - 1];
            if (char.IsWhiteSpace(c)) {
              break;
            }
            subStringLength--;
          }

          // Keep backing up until we find end-of-line or non-whitespace, to preserve leading indentation
          while (subStringLength > 0) {
            char c = longString[subStringStartIndex + subStringLength - 1];
            if (c == '\n' || !char.IsWhiteSpace(c)) {
              break;
            }
            subStringLength--;
          }

          scrollTextCell.text = longString.Substring(subStringStartIndex, subStringLength);
          subStringStartIndex += subStringLength;
        }

        currentSubString++;
      }
      yield return 0;
    }
    _CurrentRoutine = null;
  }

  private void ClearContainer() {
    foreach (Transform child in _ContentContainer) {
      GameObject.Destroy(child.gameObject);
    }
  }
}
