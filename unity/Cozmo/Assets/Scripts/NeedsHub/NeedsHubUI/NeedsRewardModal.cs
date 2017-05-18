using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class NeedsRewardModal : BaseModal {

    [SerializeField]
    private CozmoButton _ContinueButton;

    [SerializeField]
    private CozmoText _RewardsList;

    public void Start() {
      _ContinueButton.Initialize(HandleContinueButtonClicked, "rewards_continue_button", DASEventDialogName);
    }

    public void Init(NeedsReward[] rewards) {
      string str = "";
      // TODO: this will be styled based on reward type
      // rather than in one string
      for (int i = 0; i < rewards.Length; ++i) {
        str += rewards[i].rewardType + " " + rewards[i].data + "\n";
      }
      _RewardsList.text = str;
    }

    private void HandleContinueButtonClicked() {
      UIManager.CloseModal(this);
    }

  }
}