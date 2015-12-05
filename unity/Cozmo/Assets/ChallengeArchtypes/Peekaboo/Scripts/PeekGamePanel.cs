using UnityEngine;
using System.Collections;
using Cozmo.UI;

namespace Peekaboo {
  
  public class PeekGamePanel : BaseView {

    [SerializeField]
    private Anki.UI.AnkiTextLabel _PointsLabel;

    // Use this for initialization
    public void SetPoints(int points) {
      string pointsLocalized = Localization.Get(LocalizationKeys.kPeekabooLabelPoints);
      _PointsLabel.text = string.Format(Localization.GetCultureInfo(), pointsLocalized, points);
    }

    protected override void CleanUp() {

    }
  }
}
