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
  private GameObject _CostContainer;

  [SerializeField]
  private Cozmo.UI.CozmoButton _UnlockButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _StartChallengeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ViewPreReqButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  private string _ChallengeId;

  [SerializeField]
  private CanvasGroup _AlphaController;

  private int _CubesRequired;

  public void Initialize(ChallengeData challengeData) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
    _DescriptionTextLabel.text = Localization.Get(challengeData.ChallengeDescriptionLocKey);
    _ChallengeData = challengeData;
    _ChallengeIcon.SetIcon(challengeData.ChallengeIcon);
    _AvailableContainer.SetActive(true);
    _AffordableIcon.SetActive(false);
    _CubesRequired = challengeData.MinigameConfig.NumCubesRequired();
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
        string costName = itemData.GetPluralName();
        _UnlockButton.Text = Localization.Get(LocalizationKeys.kUnlockableUnlock);
        _LockedContainer.SetActive(true);
        _LockedIcon.SetActive(true);
        _UnlockedContainer.SetActive(false);
        _CostButtonLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelEmptyWithArg, cost);
        if (affordable) {
          // Can Currently Unlock but not afford
          _CurrentCostLabel.text = Localization.Get(LocalizationKeys.kLabelAvailable);
          _CurrentCostLabel.color = _AvailableColor;
          _LockedIcon.SetActive(false);
          _AffordableIcon.SetActive(true);
          _UnlockButton.Initialize(OnUpgradeClicked, "unlock_button", "challenge_details_dialog");
          _CostButtonLabel.color = _UnlockButton.TextEnabledColor;
        }
        else {
          // Can Currently Unlock and Afford
          _CurrentCostLabel.text = Localization.GetWithArgs(LocalizationKeys.kUnlockableCurrencyRequired, new object[] { costName });
          _CurrentCostLabel.color = _UnavailableColor;
          _UnlockButton.Interactable = false;
          _CostButtonLabel.color = _UnlockButton.TextDisabledColor;
        }
      }
    }
    _StartChallengeButton.Initialize(HandleStartButtonClicked, string.Format("{0}_start_button", challengeData.ChallengeID), DASEventViewName);
    _ChallengeId = challengeData.ChallengeID;
  }

  private void HandleStartButtonClicked() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      int currentNumCubes = robot.LightCubes.Count;
      if (currentNumCubes >= _CubesRequired) {
        // Don't attempt to refresh home view if we are already destroying it to start a game
        if (ChallengeStarted != null) {
          ChallengeStarted(_ChallengeId);
        }
      }
      else {
        OpenNeedCubesAlert(currentNumCubes, _CubesRequired);
      }
    }
    else {
      this.CloseView();
    }
  }

  private void OpenNeedCubesAlert(int currentCubes, int neededCubes) {
    AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab);
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(true);
    alertView.SetPrimaryButton(LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalButton,
      () => {
        UIManager.OpenView(AlertViewLoader.Instance.CubeHelpViewPrefab);
      }
    );

    alertView.TitleLocKey = LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalTitle;

    int differenceCubes = neededCubes - currentCubes;
    alertView.SetMessageArgs(new object[] {
      differenceCubes,
      ItemDataConfig.GetCubeData().GetAmountName(differenceCubes),
      Localization.Get(_ChallengeData.ChallengeTitleLocKey)
    });
    alertView.DescriptionLocKey = LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalDescription;

    this.CloseView();
  }

  private void OnUpgradeClicked() {
    UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(_ChallengeData.UnlockId.Value);
    string hexPieceId = unlockInfo.UpgradeCostItemId;
    int unlockCost = unlockInfo.UpgradeCostAmountNeeded;

    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(hexPieceId, unlockCost)) {
      playerInventory.RemoveItemAmount(hexPieceId, unlockCost);
      _UnlockButton.gameObject.SetActive(false);
      _CostContainer.gameObject.SetActive(false);
      _LockedContainer.gameObject.SetActive(false);

      UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);

      _UnlockButton.Interactable = false;
      _CostButtonLabel.color = _UnlockButton.TextDisabledColor;
      UnlockablesManager.Instance.OnUnlockComplete += HandleUnlockFromRobotResponded;
      PlayUpgradeAnimation();
    }
  }

  private void PlayUpgradeAnimation() {
    _UnlockTween = DOTween.Sequence();
    _UnlockTween.Join(_ChallengeIcon.IconImage.DOColor(Color.white, _UnlockTween_sec));
    _UnlockTween.AppendCallback(HandleUpgradeAnimationPlayed);
    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Upgrade);
  }

  //Reinitialize with new state if unlocked
  private bool _UpgradeAnimationPlayed = false;
  private bool _UnlockFromRobotResponded = false;

  private void HandleUpgradeAnimationPlayed() {
    _UpgradeAnimationPlayed = true;
    if (_UpgradeAnimationPlayed && _UnlockFromRobotResponded) {
      HandleUpgradeUnlocked();
    }
  }

  private void HandleUnlockFromRobotResponded(Anki.Cozmo.UnlockId unlockId) {
    if (unlockId == _ChallengeData.UnlockId.Value) {
      UnlockablesManager.Instance.OnUnlockComplete -= HandleUnlockFromRobotResponded;
      _UnlockFromRobotResponded = true;
      if (_UnlockFromRobotResponded && _UpgradeAnimationPlayed) {
        HandleUpgradeUnlocked();
      }
    }
  }

  private void HandleUpgradeUnlocked() {
    Initialize(_ChallengeData);

    _UpgradeAnimationPlayed = false;
    _UnlockFromRobotResponded = false;
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
  }
}
