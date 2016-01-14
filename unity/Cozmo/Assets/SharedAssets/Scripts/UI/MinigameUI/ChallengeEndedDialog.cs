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
}
