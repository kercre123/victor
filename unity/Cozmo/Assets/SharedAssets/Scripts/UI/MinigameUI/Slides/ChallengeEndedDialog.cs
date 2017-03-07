using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.UI {
  public class ChallengeEndedDialog : MonoBehaviour {

    [SerializeField]
    private int _MaxNumRewards = 3;

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
    private ChallengeDetailsUnlock _UnlockIconPrefab;

    [SerializeField]
    private ScrollRect[] _RewardIconScrollRects;

    private bool _DualLists = false;

    private ChallengeData _ChallengeConfig;

    [SerializeField]
    private GameObject[] _ScrollRectGradiants;

    public void SetupDialog(ChallengeData config) {
      _ChallengeConfig = config;
      if (_DualContainer != null) {
        _DualContainer.gameObject.SetActive(false);
      }
      if (_CenterContainer != null) {
        _CenterContainer.gameObject.SetActive(false);
      }
    }

    /// <summary>
    /// Hide subtitle and display the reward lists.
    /// </summary>
    public void DisplayRewards() {
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

      foreach (RewardedActionData earnedReward in RewardedActionManager.Instance.PendingActionRewards.Keys) {
        int count = 0;
        if (RewardedActionManager.Instance.PendingActionRewards.TryGetValue(earnedReward, out count)) {
          AddEnergyReward(earnedReward, count);
        }
      }

      bool moreThanMaxRewards = RewardedActionManager.Instance.PendingActionRewards.Count > _MaxNumRewards;
      foreach (ScrollRect rect in _RewardIconScrollRects) {
        rect.enabled = moreThanMaxRewards;
        if (moreThanMaxRewards) {
          // Scroll to the top of the rect
          rect.verticalNormalizedPosition = 1f;
        }
      }
      foreach (GameObject go in _ScrollRectGradiants) {
        go.SetActive(moreThanMaxRewards);
      }

      RewardedActionManager.Instance.NewDifficultyUnlock = -1;
      RewardedActionManager.Instance.NewSkillChange = 0;

      Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Window_Vertical_Wipe);
    }

    public void AddEnergyReward(RewardedActionData reward, int count) {
      if (_EnergyEarnedContainer != null) {
        _EnergyEarnedContainer.gameObject.SetActive(true);
      }
      IconTextLabel iconTextLabel = UIManager.CreateUIElement(_RewardIconPrefab,
                                      _EnergyEarnedContainer).GetComponent<IconTextLabel>();
      // TextLabel for amount earned
      iconTextLabel.SetText(count.ToString());

      iconTextLabel.SetDesc(Localization.Get(reward.Reward.DescriptionKey));
    }

    public void AddDifficultyUnlock(int newLevel) {
      if (_DifficultyUnlockContainer != null) {
        _DifficultyUnlockContainer.gameObject.SetActive(true);
      }

      ChallengeDetailsUnlock unlockCell = UIManager.CreateUIElement(_UnlockIconPrefab,
                                                              _DifficultyUnlockContainer).GetComponent<ChallengeDetailsUnlock>();

      unlockCell.Text = (Localization.GetWithArgs(LocalizationKeys.kRewardDescriptionNewDifficulty,
                                                     new object[] { _ChallengeConfig.DifficultyOptions[newLevel].DifficultyName }));
      unlockCell.UnlockGameContainerEnabled = true;
      unlockCell.UnlockSkillContainerEnabled = false;
    }

    public void AddSkillChange(int newLevel) {
      if (_DifficultyUnlockContainer != null) {
        _DifficultyUnlockContainer.gameObject.SetActive(true);
      }

      ChallengeDetailsUnlock unlockCell = UIManager.CreateUIElement(_UnlockIconPrefab,
                                      _DifficultyUnlockContainer).GetComponent<ChallengeDetailsUnlock>();

      string descKey = LocalizationKeys.kRewardDescriptionSkillUp;
      unlockCell.Text = (Localization.GetWithArgs(descKey, Localization.Get(_ChallengeConfig.ChallengeTitleLocKey)));
      unlockCell.UnlockGameContainerEnabled = false;
      unlockCell.UnlockSkillContainerEnabled = true;
    }

    public void HideScrollRectGradients() {
      foreach (GameObject go in _ScrollRectGradiants) {
        go.SetActive(false);
      }
    }
  }
}