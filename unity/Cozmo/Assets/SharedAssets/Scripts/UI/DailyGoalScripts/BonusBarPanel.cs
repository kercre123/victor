using UnityEngine;
using System.Collections;
using Anki.UI;
using Cozmo.UI;
using UnityEngine.UI;

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

  public void SetFriendshipBonus(float prog) {
    bool setActive = (prog > 1.0f);
    _ActiveBonusContainer.SetActive(setActive);
    _InactiveBonusContainer.SetActive(!setActive);
    if (setActive) {
      int mult = Mathf.CeilToInt(prog);
      _BonusMultText.FormattingArgs = new object[] { mult };
      _BonusProgressBar.SetProgress(prog - Mathf.Floor(prog));
      _BonusProgressBar.FillImage = DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults[mult - 2].Fill;
      _BonusProgressBg.overrideSprite = DailyGoalManager.Instance.GetFriendshipProgressConfig().BonusMults[mult - 2].Background;
    }
  }

}
