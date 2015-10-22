using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class BadgeDisplay : MonoBehaviour {
	[SerializeField]
  private Text _countLabel;

  [SerializeField]
  private RectTransform _toggleBadgeDisplay;

  private object _currentKey = null;
  private string _currentTag = null;

  public bool IsShowing()
  {
    return _toggleBadgeDisplay.gameObject.activeSelf;
  }

	// Update is called once per frame
	public void UpdateDisplayWithKey (object key) {
    _currentKey = key;
    _currentTag = null;
    bool keyExists = BadgeManager.DoesBadgeExistForKey (key);
    UpdateDisplay (keyExists, 1);
	}

  public void UpdateDisplayWithTag (string tag) {
    _currentKey = null;
    _currentTag = tag;
    int count = BadgeManager.NumBadgesWithTag (tag);
    bool showBadge = count > 0;
    UpdateDisplay (showBadge, count);
  }

  public void HideDisplay()
  {
    UpdateDisplay (false, 0);
  }

  private void UpdateDisplay(bool show, int count)
  {
    _toggleBadgeDisplay.gameObject.SetActive (show);
    
    if (_countLabel != null) {
      _countLabel.gameObject.SetActive(show);
      _countLabel.text = count.ToString();
    }
  }
}
