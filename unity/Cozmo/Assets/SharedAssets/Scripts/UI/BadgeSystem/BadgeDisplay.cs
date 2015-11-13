using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class BadgeDisplay : MonoBehaviour {
  [SerializeField]
  private Text _CountLabel;

  [SerializeField]
  private RectTransform _ToggleBadgeDisplay;

  public bool IsShowing() {
    return _ToggleBadgeDisplay.gameObject.activeSelf;
  }

  // Update is called once per frame
  public void UpdateDisplayWithKey(object key) {
    bool keyExists = BadgeManager.DoesBadgeExistForKey(key);
    UpdateDisplay(keyExists, 1);
  }

  public void UpdateDisplayWithTag(string tag) {
    int count = BadgeManager.NumBadgesWithTag(tag);
    bool showBadge = count > 0;
    UpdateDisplay(showBadge, count);
  }

  public void HideDisplay() {
    UpdateDisplay(false, 0);
  }

  private void UpdateDisplay(bool show, int count) {
    _ToggleBadgeDisplay.gameObject.SetActive(show);
    
    if (_CountLabel != null) {
      _CountLabel.gameObject.SetActive(show);
      _CountLabel.text = string.Format(Localization.GetCultureInfo(), "{0}", count);
    }
  }
}
