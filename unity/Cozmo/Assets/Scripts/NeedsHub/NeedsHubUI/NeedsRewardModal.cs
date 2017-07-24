using Anki.Cozmo;
using Cozmo.Songs;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class NeedsRewardModal : BaseModal {

    [SerializeField]
    private CozmoButton _ContinueButton;

    [SerializeField]
    private Animator _ContinueButtonAnimator;

    [SerializeField]
    private Animator _RewardBoxAnimator;

    [SerializeField]
    private GameObject _SparkRewardContainer;

    [SerializeField]
    private CozmoText _SparkRewardAmountText;

    [SerializeField]
    private GameObject _UnlockRewardContainer;

    [SerializeField]
    private CozmoImage _UnlockRewardIcon;

    [SerializeField]
    private CozmoText _RewardText;

    private int _CurrentUnlockRewardIndex = 0;
    private NeedsReward[] _RewardsToDisplay;

    private static readonly int _ContinueOpenAnimHash = Animator.StringToHash("RewardContinueButtonOpen");
    private static readonly int _ModalOpenAnimHash = Animator.StringToHash("RewardModalOpenAnimation");
    private static readonly int _ModalCloseAnimHash = Animator.StringToHash("RewardModalCloseAnimation");
    private static readonly int _StartOpenParamHash = Animator.StringToHash("StartOpen");
    private static readonly int _StartCloseParamHash = Animator.StringToHash("StartClose");

    public void Start() {
      _ContinueButton.Initialize(HandleContinueButtonClicked, "rewards_continue_button", DASEventDialogName);

      AnimatorUtil.ListenForAnimationEnd(_RewardBoxAnimator, HandleBoxBurstAnimationEnd);
    }

    public void Init(NeedsReward[] rewards) {
      _RewardsToDisplay = rewards;
      _CurrentUnlockRewardIndex = 0;
    }

    // Assumes an AnimationEventStateMachineBehavior has been placed on the box state
    private void HandleBoxBurstAnimationEnd(AnimatorStateInfo animatorStateInfo) {
      AnimatorUtil.StopListenForAnimationEnd(_RewardBoxAnimator, HandleBoxBurstAnimationEnd);

      _ContinueButtonAnimator.SetBool(_StartOpenParamHash, true);
      _ContinueButtonAnimator.SetBool(_StartCloseParamHash, false);

      UpdateRewardDisplay();
    }

    private void HandleContinueButtonClicked() {
      if (_CurrentUnlockRewardIndex < _RewardsToDisplay.Length - 1) {
        _CurrentUnlockRewardIndex++;

        _ContinueButtonAnimator.SetBool(_StartOpenParamHash, true);
        _ContinueButtonAnimator.SetBool(_StartCloseParamHash, true);
        AnimatorUtil.ListenForAnimationStart(_ContinueButtonAnimator, HandleContinueButtonOpenAnimStart);

        _RewardBoxAnimator.SetBool(_StartOpenParamHash, true);
        _RewardBoxAnimator.SetBool(_StartCloseParamHash, true);
        AnimatorUtil.ListenForAnimationStart(_RewardBoxAnimator, HandleModalOpenAnimationStart);
      }
      else {
        _ContinueButtonAnimator.SetBool(_StartOpenParamHash, false);
        _ContinueButtonAnimator.SetBool(_StartCloseParamHash, true);
        _RewardBoxAnimator.SetBool(_StartOpenParamHash, false);
        _RewardBoxAnimator.SetBool(_StartCloseParamHash, true);
        AnimatorUtil.ListenForAnimationEnd(_RewardBoxAnimator, HandleModalCloseAnimationEnd);
      }
    }

    // Assumes that a AnimationEventStateMachineBehavior has been placed on the continue open state
    private void HandleContinueButtonOpenAnimStart(AnimatorStateInfo animatorStateInfo) {
      if (_ContinueOpenAnimHash == animatorStateInfo.shortNameHash) {
        AnimatorUtil.StopListenForAnimationStart(_ContinueButtonAnimator, HandleContinueButtonOpenAnimStart);
        _ContinueButtonAnimator.SetBool(_StartOpenParamHash, false);
        _ContinueButtonAnimator.SetBool(_StartCloseParamHash, false);
      }
    }

    // Assumes that a AnimationEventStateMachineBehavior has been placed on the modal open state
    private void HandleModalOpenAnimationStart(AnimatorStateInfo animatorStateInfo) {
      if (_ModalOpenAnimHash == animatorStateInfo.shortNameHash) {
        AnimatorUtil.StopListenForAnimationStart(_RewardBoxAnimator, HandleModalOpenAnimationStart);
        _RewardBoxAnimator.SetBool(_StartOpenParamHash, false);
        _RewardBoxAnimator.SetBool(_StartCloseParamHash, false);

        UpdateRewardDisplay();
      }
    }

    // Assumes that a AnimationEventStateMachineBehavior has been placed on the modal close state
    private void HandleModalCloseAnimationEnd(AnimatorStateInfo animatorStateInfo) {
      if (_ModalCloseAnimHash == animatorStateInfo.shortNameHash) {
        AnimatorUtil.StopListenForAnimationEnd(_RewardBoxAnimator, HandleModalCloseAnimationEnd);
        UIManager.CloseModal(this);
      }
    }

    private void UpdateRewardDisplay() {
      _SparkRewardContainer.SetActive(false);
      _UnlockRewardContainer.SetActive(false);
      
      // reset reward text color in case it was changed by last reward
      _RewardText.color = UIColorPalette.NeutralTextColor;

      NeedsReward currentReward = _RewardsToDisplay[_CurrentUnlockRewardIndex];
      switch (currentReward.rewardType) {
      case NeedsRewardType.Unlock:
        DisplayUnlockReward(currentReward);
        break;
      case NeedsRewardType.Sparks:
        DisplaySparkReward(currentReward);
        break;
      case NeedsRewardType.Song:
        DisplaySongReward(currentReward);
        break;
      default:
        DAS.Error("NeedsRewardModal.UpdateRewardDisplay.RewardTypeNotImplemented",
                  currentReward.rewardType.ToString() + " not implemented!");
        break;
      }
    }

    private void DisplayUnlockReward(NeedsReward currentReward) {
      _UnlockRewardContainer.SetActive(true);
      try {
        UnlockId id = (UnlockId)System.Enum.Parse(typeof(UnlockId), currentReward.data);
        UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(id);
        _UnlockRewardIcon.sprite = unlockInfo.CoreUpgradeIcon;
        _RewardText.text = Localization.GetWithArgs(LocalizationKeys.kNeedsRewardsDialogTrickUnlocked,
                                                          Localization.Get(unlockInfo.TitleKey));
      }
      catch (System.Exception e) {
        DAS.Error("NeedsRewardModal.DisplayUnlockReward.InvalidUnlock", e.Message);
      }
    }

    private void DisplaySparkReward(NeedsReward currentReward) {
      _SparkRewardContainer.SetActive(true);
      
      if (currentReward.inventoryIsFull) {
        _RewardText.text = Localization.GetWithArgs(LocalizationKeys.kNeedsRewardsDialogSparksInventoryFull);
        _RewardText.color = UIColorPalette.WarningTextColor;
      }
      else {
        _RewardText.text = Localization.GetWithArgs(LocalizationKeys.kNeedsRewardsDialogSparksEarned);
      }
      
      int numSparksEarned = -1;
      int.TryParse(currentReward.data, out numSparksEarned);
      if (numSparksEarned != -1) {
        _SparkRewardAmountText.text = Localization.GetNumber(numSparksEarned);
      }
      else {
        DAS.Error("NeedsRewardModal.DisplaySparkReward.InvalidIntAmount",
                  string.Format("rewardType={0} data={1}", currentReward.rewardType, currentReward.data));
      }
    }

    private void DisplaySongReward(NeedsReward currentReward) {
      _UnlockRewardContainer.SetActive(true);
      try {
        UnlockId id = (UnlockId)System.Enum.Parse(typeof(UnlockId), currentReward.data);
        _UnlockRewardIcon.sprite = SongLocMap.SongIcon;
        _RewardText.text = Localization.GetWithArgs(LocalizationKeys.kNeedsRewardsDialogTrickUnlocked,
                                                    SongLocMap.GetTitleForSong(id));
      }
      catch (System.Exception e) {
        DAS.Error("NeedsRewardModal.DisplaySongReward.InvalidUnlock", e.Message);
      }
    }
  }
}