using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;
using Anki.UI;

public class ChallengeEndedDialog : MonoBehaviour {

  [SerializeField]
  private AnkiTextLabel _AdditionalInfoLabel;

  [SerializeField]
  private LayoutElement _AdditionalInfoLabelLayoutElement;

  [SerializeField]
  private HorizontalOrVerticalLayoutGroup _RewardContainer;

  [SerializeField]
  private LayoutElement _RewardContainerLayoutElement;

  [SerializeField]
  private IconTextLabel _RewardIconPrefab;

  private Transform[] _RewardIconsByStat;

  public void SetupDialog(string subtitleText) {
    _RewardIconsByStat = new Transform[(int)Anki.Cozmo.ProgressionStatType.Count];
    _RewardContainerLayoutElement.gameObject.SetActive(false);

    if (!string.IsNullOrEmpty(subtitleText)) {
      _AdditionalInfoLabelLayoutElement.gameObject.SetActive(true);
      _AdditionalInfoLabel.text = subtitleText;
    }
    else {
      _AdditionalInfoLabelLayoutElement.gameObject.SetActive(false);
    }
  }

  public void AddReward(Anki.Cozmo.ProgressionStatType progressionStat, int numberPoints) {
    _RewardContainerLayoutElement.gameObject.SetActive(true);
    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab, 
                                    _RewardContainer.transform).GetComponent<IconTextLabel>();
    iconTextLabel.SetText(Localization.GetWithArgs(LocalizationKeys.kLabelPlusCount, numberPoints));

    iconTextLabel.SetIcon(ProgressionStatConfig.Instance.GetIconForStat(progressionStat));
    _RewardIconsByStat[(int)progressionStat] = (iconTextLabel.transform);
  }

  public Transform[] GetRewardIconsByStat() {
    foreach (var reward in _RewardIconsByStat) {
      if (reward != null) {
        reward.transform.SetParent(UIManager.GetUICanvas().transform, worldPositionStays: true);
      }
    }
    return _RewardIconsByStat;
  }
}
