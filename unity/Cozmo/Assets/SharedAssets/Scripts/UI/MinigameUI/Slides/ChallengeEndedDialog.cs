using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo;
using Cozmo.UI;
using Anki.UI;
using Anki.Cozmo;

// TODO : Kill Progression Stat based display, modify to show rewards after initial results state.
// TODO : Reference experience properly, always use that energy icon for energy earnings.
// TODO : Reference unlockables properly, always use the cozmoface icon for unlocks.
public class ChallengeEndedDialog : MonoBehaviour {

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
    _DualLists = (RewardedActionManager.Instance.RewardPending && RewardedActionManager.Instance.NewDifficultyPending);
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

    if (RewardedActionManager.Instance.NewDifficultyUnlock != -1) {
      AddDifficultyUnlock(RewardedActionManager.Instance.NewDifficultyUnlock);
    }


    foreach (GameEvent eventID in RewardedActionManager.Instance.PendingActionRewards.Keys) {
      int count = 0;
      if (RewardedActionManager.Instance.PendingActionRewards.TryGetValue(eventID, out count)) {
        AddEnergyReward(eventID, count);
      }
    }
    RewardedActionManager.Instance.NewDifficultyUnlock = -1;
    
  }

  public void AddEnergyReward(GameEvent gEvent, int count) {
    if (_EnergyEarnedContainer != null) {
      _EnergyEarnedContainer.gameObject.SetActive(true);
    }
    ItemData data = ItemDataConfig.GetData(RewardedActionManager.Instance.ActionRewardID);
    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab, 
                                    _EnergyEarnedContainer).GetComponent<IconTextLabel>();
    // TextLabel for amount earned
    iconTextLabel.SetText(Localization.GetWithArgs(LocalizationKeys.kLabelPlusCount, count));

    iconTextLabel.SetIcon(data.Icon);

    iconTextLabel.SetDesc(Localization.Get(RewardedActionManager.Instance.RewardEventMap[gEvent].Description));
  }

  public void AddDifficultyUnlock(int newLevel) {
    if (_DifficultyUnlockContainer != null) {
      _DifficultyUnlockContainer.gameObject.SetActive(true);
    }

    IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab, 
                                    _DifficultyUnlockContainer).GetComponent<IconTextLabel>();
    
    iconTextLabel.SetText("");
    iconTextLabel.SetDesc(Localization.GetWithArgs(LocalizationKeys.kRewardDescriptionNewDifficulty, _ChallengeConfig.DifficultyOptions[newLevel].DifficultyName));

    iconTextLabel.SetIcon(_ChallengeConfig.ChallengeIcon);
    
  }
}
