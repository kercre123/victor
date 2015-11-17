using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class TimeTextLabel : Text {

  [SerializeField]
  private int _WarningSecondsThreshold = 5;

  [SerializeField]
  private Color _NormalTextColor = Color.white;

  [SerializeField]
  private Color _WarningTextColor = Color.red;

  public void SetTimeLeft(int secondsLeft) {
    System.TimeSpan timeSpan = TimeUtility.TimeSpanFromSeconds(secondsLeft);
    this.text = string.Format(Localization.GetCultureInfo(),
      "{0:g}", timeSpan);
    this.color = (secondsLeft > _WarningSecondsThreshold) ? _NormalTextColor : _WarningTextColor;
  }
}
