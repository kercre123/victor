using UnityEngine;
using System.Collections;

// useful for setting long bodies of text (like privacy policy).
// this is necessary because unity's text rendering is garbage and
// have a small cap (~65k characters)
public class AnkiInfiniteScrollView : MonoBehaviour {

  [SerializeField]
  private RectTransform _ContentContainer;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _AnkiInfiniteScrollTextCellPrefab;

  private string[] _StringsToDisplay;
  private int _CurrentString;
  private Coroutine _CurrentRoutine;

  private const int _kStringsToProcessPerFrame = 10;

  public void SetString(string longString) {
    ClearContainer();
    _StringsToDisplay = longString.Split('\n');
    _CurrentString = 0;

    if (_CurrentRoutine != null) {
      StopCoroutine(_CurrentRoutine);
    }
    _CurrentRoutine = StartCoroutine(CreateTextLabels());
  }

  private IEnumerator CreateTextLabels() {
    int targetIndex;
    Anki.UI.AnkiTextLabel scrollTextCell;
    while (_CurrentString < _StringsToDisplay.Length) {
      targetIndex = _CurrentString + _kStringsToProcessPerFrame;
      if (targetIndex > _StringsToDisplay.Length) {
        targetIndex = _StringsToDisplay.Length;
      }
      while (_CurrentString < targetIndex) {
        scrollTextCell = GameObject.Instantiate(_AnkiInfiniteScrollTextCellPrefab.gameObject).GetComponent<Anki.UI.AnkiTextLabel>();
        scrollTextCell.text = _StringsToDisplay[_CurrentString];
        scrollTextCell.transform.SetParent(_ContentContainer, false);
        _CurrentString++;
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
