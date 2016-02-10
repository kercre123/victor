using UnityEngine;
using System.Collections;
using Anki.UI;
using Cozmo.UI;
using UnityEngine.UI;
using System;

public class BonusBarPanel : MonoBehaviour {
  [SerializeField]
  private GameObject _ActiveBonusContainer;
  [SerializeField]
  private GameObject _InactiveBonusContainer;
  [SerializeField]
  private AnkiTextLabel _BonusMultText;
  [SerializeField]
  private ProgressBar _BonusProgressBar;
  [SerializeField]
  private Image _BonusProgressBg;
  public CanvasGroup BonusBarCanvas;

  public void SetFriendshipBonus(float prog) {
    bool setActive = (prog >= 1.0f);
    int mult = Mathf.CeilToInt(prog);
    int multIndex = mult - 2;
    if ((multIndex) >= DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults.Length) {
      multIndex = DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults.Length - 1;
    }
    else if (multIndex < 0) {
      setActive = false;
    }
    _ActiveBonusContainer.SetActive(setActive);
    _InactiveBonusContainer.SetActive(!setActive);
    if (setActive) {
      _BonusMultText.FormattingArgs = new object[] { mult };
      // Display a "Complete" Gem at 100% for that value
      if (mult == (int)prog) {
        _BonusProgressBg.overrideSprite = DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults[multIndex].Complete;
        _BonusProgressBar.SetProgress(0.0f);
      }
      else {
        _BonusProgressBg.gameObject.SetActive(true);
        _BonusProgressBar.FillImage = DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults[multIndex].Fill;
        _BonusProgressBg.overrideSprite = DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults[multIndex].Background;
        _BonusProgressBar.SetProgress(prog - Mathf.Floor(prog));
      }
    }
  }

}
