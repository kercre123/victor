using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;
using DG.Tweening;
using Cozmo.UI;
using Cozmo;
using DataPersistence;

public class ChallengeDetailsDialog : BaseView {

  public delegate void ChallengeDetailsClickedHandler(string challengeId);

  public event ChallengeDetailsClickedHandler ChallengeStarted;

  private ChallengeData _ChallengeData;

  private Sequence _UnlockTween;

  [SerializeField]
  private float _UnlockTween_sec = 0.6f;

  [SerializeField]
  private Color _AvailableColor;
  [SerializeField]
  private Color _UnavailableColor;

  /// <summary>
  /// Name of the Challenge.
  /// </summary>
  [SerializeField]
  private AnkiTextLabel _TitleTextLabel;

  /// <summary>
  /// Description of the Challenge.
  /// </summary>
  [SerializeField]
  private AnkiTextLabel _DescriptionTextLabel;

  /// <summary>
  /// Label of number of Bits Required.
  /// </summary>
  [SerializeField]
  private AnkiTextLabel _CurrentCostLabel;

  /// <summary>
  /// Costx Label next to Fragment Icon
  /// </summary>
  [SerializeField]
  private AnkiTextLabel _CostButtonLabel;

  [SerializeField]
  private IconProxy _ChallengeIcon;

  [SerializeField]
  private GameObject _LockedIcon;

  [SerializeField]
  private GameObject _AffordableIcon;

  [SerializeField]
  private GameObject _LockedContainer;

  [SerializeField]
  private GameObject _AvailableContainer;

  [SerializeField]
  private GameObject _UnlockedContainer;

  [SerializeField]
  private GameObject _UnavailableContainer;

  [SerializeField]
  private Cozmo.UI.CozmoButton _UnlockButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _StartChallengeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ViewPreReqButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  private string _ChallengeId;

  private Anki.Cozmo.UnlockId _PreReqID;
  private bool _FindPrereq = false;
  private bool _NewUnlock = false;

  [SerializeField]
  private CanvasGroup _AlphaController;

  public void Initialize(ChallengeData challengeData) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
    _DescriptionTextLabel.text = Localization.Get(challengeData.ChallengeDescriptionLocKey);
    _ChallengeData = challengeData;
    _ChallengeIcon.SetIcon(challengeData.ChallengeIcon);
    _AvailableContainer.SetActive(true);
    _UnavailableContainer.SetActive(false);
    _AffordableIcon.SetActive(false);
    if (UnlockablesManager.Instance.IsUnlocked(challengeData.UnlockId.Value)) {
      // If Ready and Unlocked
      _LockedContainer.SetActive(false);
      _LockedIcon.SetActive(false);
      _UnlockedContainer.SetActive(true);
    }
    else {
      _ChallengeIcon.SetAlpha(0.4f);
      UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(challengeData.UnlockId.Value);
      // If Available to Unlock (All Prereqs Met)
      if (UnlockablesManager.Instance.IsUnlockableAvailable(challengeData.UnlockId.Value)) {
        Cozmo.Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        bool affordable = playerInventory.CanRemoveItemAmount(unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded);

        ItemData itemData = ItemDataConfig.GetData(unlockInfo.UpgradeCostItemId);
        int cost = unlockInfo.UpgradeCostAmountNeeded;
        string costName = itemData.GetAmountName(cost);
        _UnlockButton.Text = Localization.Get(LocalizationKeys.kUnlockableUnlock);
        _LockedContainer.SetActive(true);
        _LockedIcon.SetActive(true);
        _UnlockedContainer.SetActive(false);
        _CostButtonLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelXCount, cost);
        if (affordable) {
          // Can Currently Unlock but not afford
          _CurrentCostLabel.text = Localization.Get(LocalizationKeys.kLabelAvailable);
          _CurrentCostLabel.color = _AvailableColor;
          _LockedIcon.SetActive(false);
          _AffordableIcon.SetActive(true);
          _UnlockButton.onClick.AddListener(OnUpgradeClicked);
        }
        else {
          // Can Currently Unlock and Afford
          _CurrentCostLabel.text = Localization.GetWithArgs(LocalizationKeys.kUnlockableBitsRequiredDescription, new object[] { cost, costName });
          _CurrentCostLabel.color = _UnavailableColor;
          _UnlockButton.Interactable = false;
        }
      }
      // If Unavailable
      else {

        for (int i = 0; i < unlockInfo.Prerequisites.Length; i++) {
          // if available but we haven't unlocked it yet, then it is the upgrade that is blocking us
          if (UnlockablesManager.Instance.IsUnlockableAvailable(unlockInfo.Prerequisites[i].Value) && !UnlockablesManager.Instance.IsUnlocked(unlockInfo.Prerequisites[i].Value)) {
            unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(unlockInfo.Prerequisites[i].Value);
          }
        }
        // Must Unlock X First - Hide everything show PreReq container
        _AvailableContainer.SetActive(false);
        _UnavailableContainer.SetActive(true);

        string uName = unlockInfo.TitleKey;
        if (unlockInfo.Id.Value == challengeData.UnlockId.Value) {
          // If no available prereqs, display completly unavailable text
          _DescriptionTextLabel.text = Localization.GetWithArgs(LocalizationKeys.kUnlockableUnavailableDescription, new object[] { Localization.Get(uName) });

          _ViewPreReqButton.Text = Localization.Get(LocalizationKeys.kButtonClose);
          _ViewPreReqButton.onClick.AddListener(() => {
            _FindPrereq = false;
            CloseView();
          });

        }
        else {
          // Change Description to "You need to earn [Prereq] first!
          // View Upgrade button to take you to necessary Cozmo or Play Tab and fire the appropriate dialog
          _DescriptionTextLabel.text = Localization.GetWithArgs(LocalizationKeys.kUnlockablePreReqNeededDescription, new object[] { Localization.Get(uName) });

          _ViewPreReqButton.onClick.AddListener(() => {
            _PreReqID = unlockInfo.Id.Value;
            _FindPrereq = true;
            CloseView();
          });
        }


      }
    }
    _StartChallengeButton.Initialize(HandleStartButtonClicked, string.Format("{0}_start_button", challengeData.ChallengeID), DASEventViewName);
    _ChallengeId = challengeData.ChallengeID;
  }

  private void HandleStartButtonClicked() {
    if (ChallengeStarted != null) {
      ChallengeStarted(_ChallengeId);
    }
  }


  private void OnUpgradeClicked() {
    UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(_ChallengeData.UnlockId.Value);
    string hexPieceId = unlockInfo.UpgradeCostItemId;
    int unlockCost = unlockInfo.UpgradeCostAmountNeeded;

    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(hexPieceId, unlockCost)) {
      playerInventory.RemoveItemAmount(hexPieceId, unlockCost);
      _UnlockButton.gameObject.SetActive(false);
      UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);

      _UnlockButton.Interactable = false;
      PlayUpgradeAnimation();
    }
  }

  private void PlayUpgradeAnimation() {
    _UnlockTween = DOTween.Sequence();
    _UnlockTween.Join(_ChallengeIcon.IconImage.DOColor(Color.white, _UnlockTween_sec));
    _UnlockTween.AppendCallback(HandleUpgradeUnlocked);
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Win_Shared);
  }

  //Reinitialize with new state if unlocked
  private void HandleUpgradeUnlocked() {
    Initialize(_ChallengeData);
    _NewUnlock = true;
  }

  protected override void CleanUp() {
    _StartChallengeButton.onClick.RemoveAllListeners();
  }

  protected override void ConstructOpenAnimation(Sequence openAnimation) {
    openAnimation.Append(transform.DOLocalMoveY(
      50, 0.15f).From().SetEase(Ease.OutQuad).SetRelative());
    if (_AlphaController != null) {
      _AlphaController.alpha = 0;
      openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
    }
  }

  protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    closeAnimation.Append(transform.DOLocalMoveY(
      -50, 0.15f).SetEase(Ease.OutQuad).SetRelative());
    if (_AlphaController != null) {
      closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
    }
    closeAnimation.AppendCallback(() => {
      if (UnlockablesManager.Instance.OnUnlockPopupRequested != null) {
        if (_FindPrereq) {
          UnlockablesManager.Instance.OnUnlockPopupRequested.Invoke(_PreReqID, true);
        }
        else if (_NewUnlock) {
          UnlockablesManager.Instance.OnUnlockPopupRequested.Invoke(_ChallengeData.UnlockId.Value, false);
        }
      }
    });
  }
}
