using UnityEngine;
using System.Collections;
using Cozmo.UI;
using UnityEngine.UI;

namespace FaceTracking {
  
  public class FaceTrackingGamePanel : BaseView {

    [SerializeField]
    private Anki.UI.AnkiTextLabel _PointsLabel;

    [SerializeField]
    private Image _GoalArrowRight;
    [SerializeField]
    private Image _GoalArrowLeft;

    // Use this for initialization
    public void SetPoints(int points) {
      string pointsLocalized = Localization.Get(LocalizationKeys.kPeekabooLabelPoints);
      _PointsLabel.text = string.Format(Localization.GetCultureInfo(), pointsLocalized, points);
    }

    // Arrows in display are relative to Cozmo's facing, not player's
    public void SetArrowFacing(bool left) {
      _GoalArrowLeft.gameObject.SetActive(left);
      _GoalArrowRight.gameObject.SetActive(!left);
    }

    protected override void CleanUp() {

    }
  }
}
