using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo;
using Cozmo.UI;
using Anki.UI;
using Anki.Cozmo;

public class ChallengeEndedDialog : MonoBehaviour {

  private const int kMaxRewardFields = 3;

  [SerializeField]
  private AnkiTextLabel _AdditionalInfoLabel;

  [SerializeField]
  private GameObject _AdditionalInfoLabelContainer;

  [SerializeField]
  private Transform _CenterContainer;

  [SerializeField]
  private Transform _LeftContainer;

  [SerializeField]
  private Transform _RightContainer;

  private Transform _DifficultyUnlockContainer;
  private Transform _EnergyEarnedContainer;

  [SerializeField]
  private Transform _DualContainer;

  [SerializeField]
  private IconTextLabel _RewardIconPrefab;

  [SerializeField]
  private IconTextLabel _UnlockIconPrefab;

  private bool _DualLists = false;

  private ChallengeData _ChallengeConfig;

  public void SetupDialog(string subtitleText, ChallengeData config) {
    _ChallengeConfig = config;
    if (_DualContainer != null) {
      _DualContainer.gameObject.SetActive(false);
    }
    if (_CenterContainer != null) {
      _CenterContainer.gameObject.SetActive(false);
    }

    if (!string.IsNullOrEmpty(subtitleText)) {
      if (_AdditionalInfoLabelContainer != null) {
        _AdditionalInfoLabelContainer.SetActive(true);
      }
      _AdditionalInfoLabel.text = subtitleText;
    }
    else {
      if (_AdditionalInfoLabelContainer != null) {
        _AdditionalInfoLabelContainer.SetActive(false);
      }
    }
  }

  /// <summary>
  /// Hide subtitle and display the reward lists.
  /// </summary>
  public void DisplayRewards() {
    if (_AdditionalInfoLabelContainer != null) {
      _AdditionalInfoLabelContainer.SetActive(false);
    }
    // Determine which LayoutGroup(s) to use based on if both Pending Upgrades and Pending Rewards or Just one
    // If (DualLists == false) - Use Single Vert Layout Group
    // If (DualLists == true)  - Use two Vert Layout Groups nesting in Horiz Layout Group 
    // (Unlocks to the left of me, Rewards to the Right, and here I am stuck in the middle with you)
    _DualLists = (RewardedActionManager.Instance.RewardPending && (RewardedActionManager.Instance.NewDifficultyPending || RewardedActionManager.Instance.NewSkillChangePending));
    if (_DualLists) {
      _DualContainer.gameObject.SetActive(true);
      _DifficultyUnlockContainer = _LeftContainer;
      _EnergyEarnedContainer = _RightContainer;
    }
    else {
      _CenterContainer.gameObject.SetActive(true);
      _DifficultyUnlockContainer = _CenterContainer;
      _EnergyEarnedContainer = _CenterContainer;
    }

    if (RewardedActionManager.Instance.NewDifficultyPending) {
      AddDifficultyUnlock(RewardedActionManager.Instance.NewDifficultyUnlock);
    }

    if (RewardedActionManager.Instance.NewSkillChangePending) {
      AddSkillChange(RewardedActionManager.Instance.NewSkillChange);
    }

    int rewardCells = 0;
    foreach (RewardedActionData earnedReward in RewardedActionManager.Instance.PendingActionRewards.Keys) {

      int count = 0;
      if (RewardedActionManager.Instance.PendingActionRewards.TryGetValue(earnedReward, out count)) {
        AddEnergyReward(earnedReward, count);
        rewardCells++;
      }
      // TODO: Make reward fields properly collapse into eachother by game event
      if (kMaxRewardFields <= rewardCells) {
        break;
      }
    }
    RewardedActionManager.Instance.NewDifficultyUnlock = -1;
    RewardedActionManager.Instance.NewSkillChange = 0;

  }

  public void AddEnergyReward(RewardedActionData reward, int count) {
    if (_EnergyEarnedContainer != null) {
      _EnergyEarnedContainer.gameObject.SetActive(true);
    }
    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab,
                                    _EnergyEarnedContainer).GetComponent<IconTextLabel>();
    // TextLabel for amount earned
    iconTextLabel.SetText(Localization.GetWithArgs(LocalizationKeys.kLabelPlusCount, count));

    iconTextLabel.SetDesc(Localization.Get(reward.Reward.DescriptionKey));
  }

  public void AddDifficultyUnlock(int newLevel) {
    if (_DifficultyUnlockContainer != null) {
      _DifficultyUnlockContainer.gameObject.SetActive(true);
    }

    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_UnlockIconPrefab,
                                    _DifficultyUnlockContainer).GetComponent<IconTextLabel>();

    iconTextLabel.SetText("");
    iconTextLabel.SetDesc(Localization.GetWithArgs(LocalizationKeys.kRewardDescriptionNewDifficulty,
                                                   new object[] { _ChallengeConfig.DifficultyOptions[newLevel].DifficultyName, Localization.Get(_ChallengeConfig.ChallengeTitleLocKey) }));

    iconTextLabel.SetIcon(_ChallengeConfig.ChallengeIcon);

  }

  public void AddSkillChange(int newLevel) {
    if (_DifficultyUnlockContainer != null) {
      _DifficultyUnlockContainer.gameObject.SetActive(true);
    }

    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_UnlockIconPrefab,
                                    _DifficultyUnlockContainer).GetComponent<IconTextLabel>();

    iconTextLabel.SetText("");
    string descKey = LocalizationKeys.kRewardDescriptionSkillUp;
    iconTextLabel.SetDesc(Localization.GetWithArgs(descKey, Localization.Get(_ChallengeConfig.ChallengeTitleLocKey)));
  }
}
