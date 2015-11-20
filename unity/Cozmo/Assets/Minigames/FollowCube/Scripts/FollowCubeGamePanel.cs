using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeGamePanel : BaseView {

    [SerializeField]
    private Anki.UI.AnkiTextLabel _AttemptsLeftLabel;

    // Use this for initialization
    public void SetAttemptsLeft(int attemptsLeft) {
      string attemptsLocalized = Localization.Get("followCube.label.attempts");
      _AttemptsLeftLabel.text = string.Format(Localization.GetCultureInfo(), attemptsLocalized, attemptsLeft);
    }

    protected override void CleanUp() {

    }

    protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

    }
  }

}
