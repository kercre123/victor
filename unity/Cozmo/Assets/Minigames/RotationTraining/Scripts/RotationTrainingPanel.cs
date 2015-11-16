using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RotationTrainingPanel : BaseDialog {

  [SerializeField]
  private Text _timerText;

  public void SetTimeLeft(int secondsLeft) {
    System.TimeSpan timeSpan = TimeUtility.TimeSpanFromSeconds(secondsLeft);
    _timerText.text = string.Format(Localization.GetCultureInfo(),
      "{0:g}", timeSpan);
  }

  protected override void CleanUp() {
    
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    
  }
}
