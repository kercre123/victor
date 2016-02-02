using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Cozmo.UI;
using Anki.UI;

public class ChallengeEndedDialog : BaseView {

  [SerializeField]
  private AnkiTextLabel _ChallengeTitleLabel;

  [SerializeField]
  private IconProxy _ChallengeIcon;

  [SerializeField]
  private AnkiTextLabel _PrimaryInfoLabel;

  [SerializeField]
  private AnkiTextLabel _AdditionalInfoLabel;

  [SerializeField]
  private HorizontalOrVerticalLayoutGroup _RewardContainer;

  [SerializeField]
  private IconTextLabel _RewardIconPrefab;

  protected override void CleanUp() {
    
  }

  public void SetupDialog(string titleText, Sprite titleIcon, string primaryText, string secondaryText) {
    _ChallengeTitleLabel.text = titleText;
    _ChallengeIcon.SetIcon(titleIcon);

    if (!string.IsNullOrEmpty(primaryText)) {
      _PrimaryInfoLabel.text = primaryText;
    }
    else {
      _PrimaryInfoLabel.enabled = false;
    }

    if (!string.IsNullOrEmpty(secondaryText)) {
      _AdditionalInfoLabel.text = secondaryText;
    }
    else {
      _AdditionalInfoLabel.enabled = false;
    }
  }

  public void AddReward(Anki.Cozmo.ProgressionStatType progressionStat, int numberPoints) {
    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab, _RewardContainer.transform).GetComponent<IconTextLabel>();
    iconTextLabel.SetText(string.Format(Localization.GetCultureInfo(), 
      Localization.Get(LocalizationKeys.kLabelPlusCount),
      numberPoints));

    iconTextLabel.SetIcon(ProgressionStatIconMap.Instance.GetIconForStat(progressionStat));
  }
}
