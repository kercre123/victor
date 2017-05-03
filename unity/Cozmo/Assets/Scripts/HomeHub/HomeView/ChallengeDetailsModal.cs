using Anki.UI;
using Cozmo;
using Cozmo.Challenge;
using Cozmo.UI;
using DataPersistence;
using DG.Tweening;
using UnityEngine;

public class ChallengeDetailsModal : BaseModal {

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
  private AnkiTextLegacy _TitleTextLabel;

  /// <summary>
  /// Description of the Challenge.
  /// </summary>
  [SerializeField]
  private AnkiTextLegacy _DescriptionTextLabel;

  /// <summary>
  /// Label of number of Bits Required.
  /// </summary>
  [SerializeField]
  private AnkiTextLegacy _CurrentCostLabel;

  /// <summary>
  /// Costx Label next to Fragment Icon
  /// </summary>
  [SerializeField]
  private AnkiTextLegacy _CostButtonLabel;

  [SerializeField]
  private IconProxy _ChallengeIcon;

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
  private Cozmo.UI.CozmoButtonLegacy _UnlockButton;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _StartChallengeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _ViewPreReqButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  private string _ChallengeId;

  private int _CurrentNumCubes;
  private int _CubesRequired;

  private Cozmo.HomeHub.HomeView _HomeViewInstance;

  [SerializeField]
  private HasHiccupsModal _HasHiccupModal;

  public void InitializeChallengeData(ChallengeData challengeData, Cozmo.HomeHub.HomeView homeViewInstance) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
    _DescriptionTextLabel.text = Localization.Get(challengeData.ChallengeDescriptionLocKey);
    _ChallengeData = challengeData;
    _ChallengeIcon.SetIcon(challengeData.ChallengeIcon);
    _AvailableContainer.SetActive(true);
    _AffordableIcon.SetActive(false);
    _CubesRequired = challengeData.ChallengeConfig.NumCubesRequired();
    GameEventManager.Instance.OnGameEvent += HandleGameEvent;
    if (UnlockablesManager.Instance.IsUnlocked(challengeData.UnlockId.Value)) {
      // If Ready and Unlocked
      _LockedContainer.SetActive(false);
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
        _UnlockedContainer.SetActive(false);
        _CostButtonLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelEmptyWithArg, cost);
        if (affordable) {
          // Can Currently Unlock but not afford
          _CurrentCostLabel.text = Localization.Get(LocalizationKeys.kLabelAvailable);
          _CurrentCostLabel.color = _AvailableColor;
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
    // If we are initializing basedon an unlock refresh instead of a typical initialize, don't add additional callbacks to StartChallenge
    // TODO : Post Launch, investigate change to AnkiButton Initialize that assumes Initialize is creating a "fresh" button that wipes all
    // existing callbacks so it matches more clearly with expected functionality and usage patterns.
    if (!_UnlockFromRobotResponded) {
      _StartChallengeButton.Initialize(HandleStartButtonClicked, string.Format("{0}_start_button", challengeData.ChallengeID), DASEventDialogName);
    }
    _ChallengeId = challengeData.ChallengeID;
    _HomeViewInstance = homeViewInstance;
  }

  // When we unlock something new or trigger a free play goal, if this would cause a loot view sequence to trigger,
  // then close the view for the sequence to run instead of refreshing it.
  private void HandleGameEvent(GameEventWrapper gameEvent) {
    if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnDailyGoalCompleted) {
      CloseDialogImmediately();
    }
  }

  private void HandleStartButtonClicked() {
    _StartChallengeButton.Interactable = false;
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      int currentNumCubes = robot.LightCubes.Count;
      if (currentNumCubes >= _CubesRequired) {
        // Don't attempt to refresh home view if we are already destroying it to start a game
        if (ChallengeStarted != null) {
          // don't hit the sides of the charger if he's still driving off.
          if (robot.CurrentBehaviorClass == Anki.Cozmo.BehaviorClass.DriveOffCharger) {
            OpenCozmoNotReadyAlert();
          }
          else if (robot.CurrentBehaviorClass == Anki.Cozmo.BehaviorClass.ReactToRobotShaken) {
            OpenCozmoDizzyAlert();
          }
          else if (robot.TreadState != Anki.Cozmo.OffTreadsState.OnTreads) {
            OpenCozmoNotOnTreadsAlert();
          }
          else if (robot.HasHiccups) {
            _HasHiccupModal.OpenCozmoHasHiccupsAlert(this.PriorityData, HandleEdgeCaseAlertClosed);
          }
          else {
            ChallengeStarted(_ChallengeId);
          }
        }
      }
      else {
        _HomeViewInstance.OpenNeedCubesAlert(currentNumCubes, _CubesRequired, Localization.Get(_ChallengeData.ChallengeTitleLocKey));
        this.CloseDialog();
      }
    }
    else {
      this.CloseDialog();
    }
  }

  private void HandleEdgeCaseAlertClosed() {
    if (this != null) {
      _StartChallengeButton.Interactable = true;
    }
  }

  // Cozmo isn't done driving off the charger.
  private void OpenCozmoNotReadyAlert() {
    var cozmoNotReadyData = new AlertModalData("cozmo_driving_off_charger_alert",
                                               LocalizationKeys.kChallengeDetailsCozmoIsStillWakingUpModalTitle,
                                               LocalizationKeys.kChallengeDetailsCozmoIsStillWakingUpModalDescription,
                                               new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                               dialogCloseAnimationFinishedCallback: HandleEdgeCaseAlertClosed,
                                               timeoutSec: 10.0f);

    UIManager.OpenAlert(cozmoNotReadyData, ModalPriorityData.CreateSlightlyHigherData(this.PriorityData));
    this.CloseDialog();
  }

  private void OpenCozmoNotOnTreadsAlert() {
    var cozmoNotOnTreadsData = new AlertModalData("cozmo_off_treads_alert",
                                                  LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsTitle,
                                                  LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsDescription,
                                                  new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                                  dialogCloseAnimationFinishedCallback: HandleEdgeCaseAlertClosed);

    UIManager.OpenAlert(cozmoNotOnTreadsData, ModalPriorityData.CreateSlightlyHigherData(this.PriorityData));
  }

  private void OpenCozmoDizzyAlert() {
    var cozmoDizzyData = new AlertModalData("cozmo_dizzy_alert",
                                            LocalizationKeys.kChallengeDetailsCozmoDizzyTitle,
                                            LocalizationKeys.kChallengeDetailsCozmoDizzyDescription,
                                            new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                            dialogCloseAnimationFinishedCallback: HandleEdgeCaseAlertClosed);

    UIManager.OpenAlert(cozmoDizzyData, ModalPriorityData.CreateSlightlyHigherData(this.PriorityData));
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
      DAS.Event("meta.app_unlock", unlockInfo.Id.Value.ToString(), DASUtil.FormatExtraData(unlockCost.ToString()));
      _UnlockButton.Interactable = false;
      _CostButtonLabel.color = _UnlockButton.TextDisabledColor;
      UnlockablesManager.Instance.OnUnlockComplete += HandleUnlockFromRobotResponded;
      PlayUpgradeAnimation();
    }
  }

  private void PlayUpgradeAnimation() {
    if (_UnlockTween != null) {
      _UnlockTween.Kill();
    }
    _UnlockTween = DOTween.Sequence();
    _UnlockTween.Join(_ChallengeIcon.IconImage.DOColor(Color.white, _UnlockTween_sec));
    _UnlockTween.AppendCallback(HandleUpgradeAnimationPlayed);
    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Cozmo_Upgrade);
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
    DAS.Event("meta.app_unlock.unlockResultConfirmed", unlockId.ToString());
    if (unlockId == _ChallengeData.UnlockId.Value) {
      UnlockablesManager.Instance.OnUnlockComplete -= HandleUnlockFromRobotResponded;
      _UnlockFromRobotResponded = true;
      if (_UnlockFromRobotResponded && _UpgradeAnimationPlayed) {
        HandleUpgradeUnlocked();
      }
    }
  }

  private void HandleUpgradeUnlocked() {
    InitializeChallengeData(_ChallengeData, _HomeViewInstance);

    _UpgradeAnimationPlayed = false;
    _UnlockFromRobotResponded = false;
  }

  protected override void CleanUp() {
    _StartChallengeButton.onClick.RemoveAllListeners();
    GameEventManager.Instance.OnGameEvent -= HandleGameEvent;

    if (_UnlockTween != null) {
      _UnlockTween.Kill();
    }
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
