using UnityEngine;
using System.Collections.Generic;

// useful for setting long bodies of text (like privacy policy).
// this is necessary because unity's text rendering is garbage and
// have a small cap (~65k characters)
public class AnkiInfiniteScrollView : MonoBehaviour {

  [SerializeField]
  private RectTransform _ContentContainer;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _AnkiInfiniteScrollTextCellPrefab;

  public void SetString(string longString) {
    ClearContainer();
    string[] splitStrings = longString.Split('\n');
    foreach (string singleString in splitStrings) {
      UnityEngine.UI.Text scrollTextCell = GameObject.Instantiate(_AnkiInfiniteScrollTextCellPrefab.gameObject).GetComponent<UnityEngine.UI.Text>();
      scrollTextCell.text = singleString;
      scrollTextCell.transform.SetParent(_ContentContainer, false);
    }
  }

  private void ClearContainer() {
    foreach (Transform child in _ContentContainer) {
      GameObject.Destroy(child.gameObject);
    }
  }
}
