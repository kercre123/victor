using UnityEngine;
using System.Collections;
using Anki.UI;
using Cozmo.UI;

public class BonusBarPanel : MonoBehaviour {
  [SerializeField]
  private GameObject _ActiveBonusContainer;
  [SerializeField]
  private GameObject _InactiveBonusContainer;
  [SerializeField]
  private AnkiTextLabel _BonusMultText;
  [SerializeField]
  private ProgressBar _BonusProgressBar;

  public void SetFriendshipBonus(float prog) {
    bool setActive = (prog > 1.0f);
    _ActiveBonusContainer.SetActive(setActive);
    _InactiveBonusContainer.SetActive(!setActive);
    if (setActive) {
      int mult = Mathf.CeilToInt(prog);
      _BonusMultText.text = string.Format("x{0}", mult);
      _BonusProgressBar.SetProgress(prog - Mathf.Floor(prog));
      // TODO : Set ProgressBar Image based on top mult earned.
    }
  }

}
