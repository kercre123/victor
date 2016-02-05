using UnityEngine;
using UnityEngine.UI;
using System.Collections;
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

  public void SetupDialog(string subtitleText) {
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
    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab, _RewardContainer.transform).GetComponent<IconTextLabel>();
    iconTextLabel.SetText(string.Format(Localization.GetCultureInfo(), 
      Localization.Get(LocalizationKeys.kLabelPlusCount),
      numberPoints));

    iconTextLabel.SetIcon(ProgressionStatIconMap.Instance.GetIconForStat(progressionStat));
  }
}
