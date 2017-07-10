using Cozmo.Challenge;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.UI {
  public class ChallengeEndedDialog : MonoBehaviour {

    [SerializeField]
    private Transform _CenterContainer;

    [SerializeField]
    private ChallengeDetailsUnlock _UnlockIconPrefab;

    private ChallengeData _ChallengeConfig;

    public void SetupDialog(ChallengeData config) {
      _ChallengeConfig = config;
    }

    /// <summary>
    /// Hide subtitle and display the reward lists.
    /// </summary>
    public void DisplayRewards() {
      if (RewardedActionManager.Instance.NewDifficultyPending) {
        AddDifficultyUnlock(RewardedActionManager.Instance.NewDifficultyUnlock);
      }

      if (RewardedActionManager.Instance.NewSkillChangePending) {
        AddSkillChange(RewardedActionManager.Instance.NewSkillChange);
      }

      RewardedActionManager.Instance.NewDifficultyUnlock = -1;
      RewardedActionManager.Instance.NewSkillChange = 0;

      Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Window_Vertical_Wipe);
    }

    private void AddDifficultyUnlock(int newLevel) {
      ChallengeDetailsUnlock unlockCell = UIManager.CreateUIElement(_UnlockIconPrefab,
                                                              _CenterContainer).GetComponent<ChallengeDetailsUnlock>();

      unlockCell.Text = (Localization.GetWithArgs(LocalizationKeys.kRewardDescriptionNewDifficulty,
                                                     new object[] { _ChallengeConfig.DifficultyOptions[newLevel].DifficultyName }));
      unlockCell.UnlockGameContainerEnabled = true;
      unlockCell.UnlockSkillContainerEnabled = false;
    }

    private void AddSkillChange(int newLevel) {
      ChallengeDetailsUnlock unlockCell = UIManager.CreateUIElement(_UnlockIconPrefab,
                                      _CenterContainer).GetComponent<ChallengeDetailsUnlock>();

      string descKey = LocalizationKeys.kRewardDescriptionSkillUp;
      unlockCell.Text = (Localization.GetWithArgs(descKey, Localization.Get(_ChallengeConfig.ChallengeTitleLocKey)));
      unlockCell.UnlockGameContainerEnabled = false;
      unlockCell.UnlockSkillContainerEnabled = true;
    }
  }
}