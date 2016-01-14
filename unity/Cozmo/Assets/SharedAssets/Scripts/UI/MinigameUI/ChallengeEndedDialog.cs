using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Cozmo.UI;
using Anki.UI;

public class ChallengeEndedDialog : BaseView {

  [SerializeField]
  private IconTextLabel _ChallengeTitleLabel;

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
    _ChallengeTitleLabel.SetText(titleText);
    _ChallengeTitleLabel.SetIcon(titleIcon);

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

    // TODO: Set icon
  }
}
