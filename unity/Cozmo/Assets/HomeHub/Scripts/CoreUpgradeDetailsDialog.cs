using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using Anki.Cozmo;
using DataPersistence;
using Cozmo;
using DG.Tweening;
using System.Collections.Generic;
using Cozmo.HomeHub;

public class CoreUpgradeDetailsDialog : BaseView {

  public delegate void CoreUpgradeRequestedHandler(UnlockableInfo info);

  [SerializeField]
  private CanvasGroup _AlphaController;

  [SerializeField]
  private GameObject _ContentContainer;

  [SerializeField]
  private AnkiTextLabel _UnlockableNameLabel;

  [SerializeField]
  private AnkiTextLabel _UnlockableDescriptionLabel;

  [SerializeField]
  private GameObject _ButtonPromptContainer;

  [SerializeField]
  private GameObject _AffordableHighlightContainer;

  [SerializeField]
  private AnkiTextLabel _ButtonPromptTitle;

  [SerializeField]
  private AnkiTextLabel _ButtonPromptDescription;

  [SerializeField]
  private Text _SparkButtonCostLabel;

  [SerializeField]
  private Text _UnlockButtonCostLabel;

  [SerializeField]
  private AnkiTextLabel _RequestTrickButtonLabel;

  [SerializeField]
  private GameObject _RequestTrickButtonIcons;

  [SerializeField]
  private Image _UnlockableIcon;

  [SerializeField]
  private AnkiTextLabel _CubesRequiredLabel;

  [SerializeField]
  private GameObject _UnlockUpgradeButtonContainer;

  [SerializeField]
  private CozmoButton _UnlockUpgradeButton;

  [SerializeField]
  private AnkiTextLabel _FragmentInventoryLabel;
  [SerializeField]
  private GameObject _FragmentInventoryContainer;

  [SerializeField]
  private GameObject _RequestTrickButtonContainer;

  [SerializeField]
  private CozmoButton _RequestTrickButton;

  [SerializeField]
  private AnkiTextLabel _SparksInventoryLabel;
  [SerializeField]
  private GameObject _SparksInventoryContainer;

  [SerializeField]
  private GameObject _SparkSpinner;

  [SerializeField]
  private float _UpgradeTween_sec = 0.6f;

  [SerializeField]
  private GameObject _SparkImagePrefab;
  [SerializeField]
  private GameObject _DimmerPrefab;
  [SerializeField]
  private AnkiTextLabel _SparksButtonDescriptionLabel;

  private CanvasGroup _DimBackgroundInstance = null;

  private UnlockableInfo _UnlockInfo;

  private CoreUpgradeRequestedHandler _ButtonCostPaidSuccessCallback;

  private Sequence _UpgradeTween;
  private bool _ConfirmedQuit = false;

  [SerializeField]
  private Color _UpgradeAvailableColor;
  [SerializeField]
  private Color _SparkAvailableColor;
  [SerializeField]
  private Color _UnavailableColor;

  public void Initialize(UnlockableInfo unlockInfo, CozmoUnlocksPanel.CozmoUnlockState unlockState, CoreUpgradeRequestedHandler buttonCostPaidCallback) {
    _UnlockInfo = unlockInfo;
    _ButtonCostPaidSuccessCallback = buttonCostPaidCallback;

    _UnlockUpgradeButtonContainer.gameObject.SetActive(false);
    _RequestTrickButtonContainer.gameObject.SetActive(false);
    _ButtonPromptContainer.SetActive(false);
    _AffordableHighlightContainer.gameObject.SetActive(false);
    // QA testing jumps around during stages. As a failsafe just give the amount needed since they can't exist.
    Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

    // No quitting until they've finished onboarding
    if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Upgrades)) {
      if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
        _OptionalCloseDialogButton.gameObject.SetActive(false);
      }
      if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
        if (!playerInventory.CanRemoveItemAmount(unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded)) {
          playerInventory.AddItemAmount(unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded);
        }
      }
      else if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked) {
        if (!playerInventory.CanRemoveItemAmount(unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded)) {
          playerInventory.AddItemAmount(unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded);
        }
      }
    }

    if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked) {
      _UnlockableIcon.color = Color.white;
      if (unlockInfo.UnlockableType == UnlockableType.Action) {
        _RequestTrickButtonContainer.gameObject.SetActive(true);
        _FragmentInventoryContainer.gameObject.SetActive(false);
        _SparksInventoryContainer.gameObject.SetActive(true);
        UpdateAvailableCostLabels(unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded, LocalizationKeys.kSparksSparkCozmo, unlockInfo.SparkButtonDescription);
        SetupButton(_RequestTrickButton, StartSparkUnlock, "request_trick_button",
                    unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded, _SparksInventoryLabel, true);

        _SparkButtonCostLabel.text = string.Format("{0}", unlockInfo.RequestTrickCostAmountNeeded);
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.SparkEnded>(HandleSparkEnded);
      }
    }
    else if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
      _UnlockUpgradeButtonContainer.gameObject.SetActive(true);
      _FragmentInventoryContainer.gameObject.SetActive(true);
      _SparksInventoryContainer.gameObject.SetActive(false);
      UpdateAvailableCostLabels(unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded, LocalizationKeys.kUnlockableAvailable, LocalizationKeys.kUnlockableBitsRequiredEarnMore);
      SetupButton(_UnlockUpgradeButton, OnUpgradeClicked, "request_upgrade_button",
                  unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded, _FragmentInventoryLabel, false);

      _UnlockButtonCostLabel.text = string.Format("{0}", unlockInfo.UpgradeCostAmountNeeded);
      _UnlockableIcon.color = new Color(1.0F, 1.0F, 1.0F, 0.5F);
    }

    _UnlockableNameLabel.text = Localization.Get(unlockInfo.TitleKey);
    _UnlockableDescriptionLabel.text = Localization.Get(unlockInfo.DescriptionKey);

    _UnlockableIcon.sprite = unlockInfo.CoreUpgradeOverlayIcon;
    _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesNeeded,
      unlockInfo.CubesRequired,
      ItemDataConfig.GetCubeData().GetAmountName(unlockInfo.CubesRequired));

    UpdateState();

  }

  private void SetupButton(CozmoButton button, UnityEngine.Events.UnityAction buttonCallback,
                           string dasButtonName, string costItemId, int costAmount, AnkiTextLabel inventoryLabel, bool spark) {
    button.Initialize(buttonCallback, dasButtonName, this.DASEventViewName);

    Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    button.Interactable = playerInventory.CanRemoveItemAmount(costItemId, costAmount);

    ItemData itemData = ItemDataConfig.GetData(costItemId);
    if (spark) {
      if (costAmount == 1) {
        button.Text = Localization.Get (LocalizationKeys.kSparksSpark);
      } else {
        button.Text = Localization.Get (LocalizationKeys.kSparksSparkPlural);
      }
    }
    else {
      button.Text = Localization.Get(LocalizationKeys.kUnlockableUnlock);
    }

    inventoryLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelTotalCount,
      itemData.GetPluralName(),
      playerInventory.GetItemAmount(costItemId));
  }

  protected override void HandleUserClose() {

    if (RobotEngineManager.Instance.CurrentRobot.IsSparked) {
      CreateConfirmQuitAlert();
    }
    else {
      base.HandleUserClose();
    }
  }

  private void CreateConfirmQuitAlert() {
    // Open confirmation dialog instead
    AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab);
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonStaySparked, HandleStaySparked, Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Click_Back));
    alertView.SetSecondaryButton(LocalizationKeys.kButtonLeave, HandleLeaveSpark);
    alertView.TitleLocKey = LocalizationKeys.kSparksSparkConfirmQuit;
    alertView.DescriptionLocKey = LocalizationKeys.kSparksSparkConfirmQuitDescription;
    // Listen for dialog close
    alertView.ViewCloseAnimationFinished += HandleQuitViewClosed;
    _ConfirmedQuit = false;
  }

  private void HandleQuitViewClosed() {
    if (_ConfirmedQuit) {
      CloseView();
    }
    _ConfirmedQuit = false;
  }

  private void HandleStaySparked() {
    _ConfirmedQuit = false;
  }

  private void HandleLeaveSpark() {
    _ConfirmedQuit = true;
  }

  private void UpdateState() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot.IsSparked && robot.SparkUnlockId == _UnlockInfo.Id.Value) {
      _RequestTrickButton.Interactable = false;
    }
    else {
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

      if (UnlockablesManager.Instance.IsUnlocked(_UnlockInfo.Id.Value)) {
        _RequestTrickButton.Interactable = playerInventory.CanRemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);

        if (_RequestTrickButton.Interactable) {
          _SparkButtonCostLabel.color = _RequestTrickButton.TextEnabledColor;
          _ButtonPromptTitle.color = _SparkAvailableColor;
          UpdateAvailableCostLabels(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded, LocalizationKeys.kSparksSparkCozmo, _UnlockInfo.SparkButtonDescription);
        }
        else {
          _SparkButtonCostLabel.color = _RequestTrickButton.TextDisabledColor;
          _ButtonPromptTitle.color = _UnavailableColor;
          UpdateAvailableCostLabels(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded, LocalizationKeys.kSparksNotEnoughSparksTitle, LocalizationKeys.kSparksNotEnoughSparksDesc);
        }

      }
      else if (UnlockablesManager.Instance.IsUnlockableAvailable(_UnlockInfo.Id.Value)) {
        _UnlockUpgradeButton.Interactable = playerInventory.CanRemoveItemAmount(_UnlockInfo.UpgradeCostItemId, _UnlockInfo.UpgradeCostAmountNeeded);
        if (_UnlockUpgradeButton.Interactable) {
          _UnlockButtonCostLabel.color = _UnlockUpgradeButton.TextEnabledColor;
          _ButtonPromptTitle.color = _UpgradeAvailableColor;
          UpdateAvailableCostLabels(_UnlockInfo.UpgradeCostItemId, _UnlockInfo.UpgradeCostAmountNeeded, LocalizationKeys.kUnlockableAvailable, LocalizationKeys.kUnlockableBitsRequiredDescription);
        }
        else {
          _UnlockButtonCostLabel.color = _UnlockUpgradeButton.TextDisabledColor;
          _ButtonPromptTitle.color = _UnavailableColor;
          UpdateAvailableCostLabels(_UnlockInfo.UpgradeCostItemId, _UnlockInfo.UpgradeCostAmountNeeded, LocalizationKeys.kUnlockableBitsRequiredTitle, LocalizationKeys.kUnlockableBitsRequiredEarnMore);
        }

      }
    }

  }

  private void OnUpgradeClicked() {
    string hexPieceId = _UnlockInfo.UpgradeCostItemId;
    int unlockCost = _UnlockInfo.UpgradeCostAmountNeeded;

    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(hexPieceId, unlockCost)) {
      playerInventory.RemoveItemAmount(hexPieceId, unlockCost);
      _UnlockUpgradeButtonContainer.gameObject.SetActive(false);

      if (_ButtonCostPaidSuccessCallback != null) {
        _ButtonCostPaidSuccessCallback(_UnlockInfo);
      }

      _UnlockUpgradeButton.Interactable = false;
      PlayUpgradeAnimation();
    }
  }

  private void PlayUpgradeAnimation() {
    _UpgradeTween = DOTween.Sequence();
    _UpgradeTween.Join(_UnlockableIcon.DOColor(Color.white, _UpgradeTween_sec));
    _UpgradeTween.AppendCallback(ResolveOnNewUnlock);
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Win_Shared);
  }

  private void ResolveOnNewUnlock() {
    UpdateState();
    // Go back and look at your accomplishment
    CloseView();
  }

  private void UpdateAvailableCostLabels(string itemID, int cost, string promptLabelKey, string costLabelKey) {
    _ButtonPromptContainer.SetActive(true);
    _ButtonPromptTitle.text = Localization.Get(promptLabelKey);
    _ButtonPromptDescription.gameObject.SetActive(true);
    ItemData itemData = ItemDataConfig.GetData(itemID);
    string costName = Localization.Get(itemData.GetAmountName(cost));
    _ButtonPromptDescription.text = Localization.GetWithArgs(costLabelKey, new object[] { cost, costName });
    _SparkButtonCostLabel.text = string.Format("{0}", cost);
  }

  protected override void Update() {
    base.Update();

    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null && robot.IsSparked && robot.SparkUnlockId == _UnlockInfo.Id.Value) {
      _ButtonPromptDescription.gameObject.SetActive(false);
      _ButtonPromptTitle.gameObject.SetActive(true);
      _ButtonPromptTitle.text = Localization.Get(LocalizationKeys.kSparksSparked);
    }

    if (robot != null) {
      _SparkSpinner.gameObject.SetActive(robot.IsSparked);
      _SparkButtonCostLabel.gameObject.SetActive(!robot.IsSparked);
      _RequestTrickButtonLabel.gameObject.SetActive(!robot.IsSparked && _RequestTrickButton.IsActive());
      _RequestTrickButtonIcons.SetActive(!robot.IsSparked && _RequestTrickButton.IsActive());
    }

  }

  private void UpdateInventoryLabel(string itemId, AnkiTextLabel label) {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    ItemData itemData = ItemDataConfig.GetData(itemId);
    label.text = Localization.GetWithArgs(LocalizationKeys.kLabelTotalCount,
                                          itemData.GetPluralName(),
                                          playerInventory.GetItemAmount(itemId));
  }

  private void HandleSparkEnded(object message) {
    // Only fire the game event when we receive the spark ended message, rewards are only applied
    // when COMPLETING a sparked action (or timing out). View includes a warning dialog for exiting.
    GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnUnlockableSparked, _UnlockInfo.Id.Value));
    StopSparkUnlock();
  }

  private void StartSparkUnlock() {
    if (UnlockablesManager.Instance.OnSparkStarted != null) {
      UnlockablesManager.Instance.OnSparkStarted.Invoke(_UnlockInfo.Id.Value);
    }
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    TimelineEntryData sess = DataPersistenceManager.Instance.CurrentSession;

    // Inventory valid was already checked when the button was initialized.
    playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
    UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksInventoryLabel);

    if (sess.SparkCount.ContainsKey(_UnlockInfo.Id.Value)) {
      sess.SparkCount[_UnlockInfo.Id.Value]++;
    }
    else {
      sess.SparkCount.Add(_UnlockInfo.Id.Value, 1);
    }

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Spark_Button_Loop_Stop);
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Spark_Launch);
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Spark);
    RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);
    UpdateState();
    DataPersistenceManager.Instance.Save();
  }

  private void StopSparkUnlock() {
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
    if (RobotEngineManager.Instance.CurrentRobot != null){
      if (RobotEngineManager.Instance.CurrentRobot.IsSparked){
        RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
      }

      UpdateState();
    }

    if (UnlockablesManager.Instance.OnSparkComplete != null) {
      UnlockablesManager.Instance.OnSparkComplete.Invoke(this);
    }
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.SparkEnded>(HandleSparkEnded);
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Spark_Button_Loop_Stop);
    StopSparkUnlock();
    // Because of a bug within DOTween Fades don't release even after being killed, so clean up
    if (_DimBackgroundInstance != null) {
      Destroy(_DimBackgroundInstance.gameObject);
    }
    if (_UpgradeTween != null) {
      _UpgradeTween.Kill();
    }
    HomeHub.Instance.HomeViewInstance.CheckForRewardSequence();
  }

  protected override void ConstructOpenAnimation(Sequence openAnimation) {
    openAnimation.Append(_ContentContainer.transform.DOLocalMoveY(
      50, 0.15f).From().SetEase(Ease.OutQuad).SetRelative());
    if (_AlphaController != null) {
      _AlphaController.alpha = 0;
      openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
    }
  }

  protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    closeAnimation.Append(_ContentContainer.transform.DOLocalMoveY(
      -50, 0.15f).SetEase(Ease.OutQuad).SetRelative());
    if (_AlphaController != null) {
      closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
    }
  }
}
