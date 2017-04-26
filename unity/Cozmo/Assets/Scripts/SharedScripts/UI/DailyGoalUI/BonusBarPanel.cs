using UnityEngine;
using System.Collections;
using Anki.UI;
using Cozmo.UI;
using UnityEngine.UI;
using System;

// TODO : Remove entirely or reward based on new design
public class BonusBarPanel : MonoBehaviour {
  [SerializeField]
  private GameObject _ActiveBonusContainer;
  [SerializeField]
  private GameObject _InactiveBonusContainer;
  [SerializeField]
  private AnkiTextLegacy _BonusMultText;
  [SerializeField]
  private ProgressBar _BonusProgressBar;
  [SerializeField]
  private Image _BonusProgressBg;
  public CanvasGroup BonusBarCanvas;

  public void SetFriendshipBonus(float prog) {
    bool setActive = (prog >= 1.0f);

    int mult = Mathf.CeilToInt(prog);
    /*
    int multIndex = mult - 2;

    if ((multIndex) >= DailyGoalManager.Instance.GetDailyGoalGenConfig().BonusMults.Length) {
      multIndex = DailyGoalManager.Instance.GetDailyGoalGenConfig().BonusMults.Length - 1;
    }
    else if (multIndex < 0) {
      setActive = false;
    }*/
    setActive = false;
    _ActiveBonusContainer.SetActive(setActive);
    _InactiveBonusContainer.SetActive(!setActive);
    if (setActive) {
      _BonusMultText.FormattingArgs = new object[] { mult };
      // Display a "Complete" Gem at 100% for that value
      if (mult == (int)prog) {
        //_BonusProgressBg.overrideSprite = DailyGoalManager.Instance.GetDailyGoalGenConfig().BonusMults[multIndex].Complete;
        _BonusProgressBar.SetTargetAndAnimate(0.0f);
      }
      else {
        _BonusProgressBg.gameObject.SetActive(true);
        //_BonusProgressBar.FillImage = DailyGoalManager.Instance.GetDailyGoalGenConfig().BonusMults[multIndex].Fill;
        //_BonusProgressBg.overrideSprite = DailyGoalManager.Instance.GetDailyGoalGenConfig().BonusMults[multIndex].Background;
        _BonusProgressBar.SetTargetAndAnimate(prog - Mathf.Floor(prog));
      }
    }
  }

}
