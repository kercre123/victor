using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DataPersistence;
using Cozmo;
using DG.Tweening;

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
  private Image _UnlockableIcon;

  [SerializeField]
  private Image _UnlockableTintBackground;

  [SerializeField]
  private Image _UnlockableActionIndicator;

  [SerializeField]
  private AnkiTextLabel _CubesRequiredLabel;

  [SerializeField]
  private GameObject _UnlockUpgradeButtonContainer;

  [SerializeField]
  private CozmoButton _UnlockUpgradeButton;

  [SerializeField]
  private AnkiTextLabel _FragmentInventoryLabel;

  [SerializeField]
  private GameObject _RequestTrickButtonContainer;

  [SerializeField]
  private CozmoButton _RequestTrickButton;

  [SerializeField]
  private AnkiTextLabel _SparksInventoryLabel;

  [SerializeField]
  private float _UpgradeTween_sec = 0.6f;

  private UnlockableInfo _UnlockInfo;

  private CoreUpgradeRequestedHandler _ButtonCostPaidSuccessCallback;

  private Sequence _UpgradeTween;

  private bool _NewUnlock = false;

  public void Initialize(UnlockableInfo unlockInfo, CozmoUnlocksPanel.CozmoUnlockState unlockState, CoreUpgradeRequestedHandler buttonCostPaidCallback) {
    _UnlockInfo = unlockInfo;
    _ButtonCostPaidSuccessCallback = buttonCostPaidCallback;

    _UnlockUpgradeButtonContainer.gameObject.SetActive(false);
    _RequestTrickButtonContainer.gameObject.SetActive(false);

    if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked) {
      _UnlockableIcon.color = Color.white;
      _UnlockableTintBackground.color = UIColorPalette.GetUpgradeTintData(unlockInfo.CoreUpgradeTintColorName).TintColor;
      if (unlockInfo.UnlockableType == UnlockableType.Action) {
        // TODO: Once request tricks is working show the buttons
        // _RequestTrickButtonContainer.gameObject.SetActive(true);
        SetupButton(_RequestTrickButton, OnSparkClicked, "request_trick_button",
          unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded, _SparksInventoryLabel);
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.SparkUnlockEnded>(HandleSparkUnlockEnded);
      }
    }
    else if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
      _UnlockUpgradeButtonContainer.gameObject.SetActive(true);
      SetupButton(_UnlockUpgradeButton, OnUpgradeClicked, "request_upgrade_button",
        unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded, _FragmentInventoryLabel);
      _UnlockableIcon.color = Color.gray;
      _UnlockableTintBackground.color = Color.gray;
    }

    _UnlockableNameLabel.text = Localization.Get(unlockInfo.TitleKey);
    _UnlockableDescriptionLabel.text = Localization.Get(unlockInfo.DescriptionKey);

    _UnlockableIcon.sprite = unlockInfo.CoreUpgradeIcon;
    _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesNeeded,
      unlockInfo.CubesRequired,
      ItemDataConfig.GetCubeData().GetAmountName(unlockInfo.CubesRequired));

    _UnlockableActionIndicator.gameObject.SetActive(unlockInfo.UnlockableType == UnlockableType.Action);

    UpdateState();
  }

  private void SetupButton(CozmoButton button, UnityEngine.Events.UnityAction buttonCallback,
                           string dasButtonName, string costItemId, int costAmount, AnkiTextLabel inventoryLabel) {
    button.Initialize(buttonCallback, dasButtonName, this.DASEventViewName);

    Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    button.Interactable = playerInventory.CanRemoveItemAmount(costItemId, costAmount);

    ItemData itemData = ItemDataConfig.GetData(costItemId);
    button.Text = Localization.GetWithArgs(LocalizationKeys.kLabelSimpleCount,
      costAmount,
      itemData.GetAmountName(costAmount));
    inventoryLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelColonCount,
      itemData.GetPluralName(),
      playerInventory.GetItemAmount(costItemId));
  }

  private void UpdateState() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot.IsSparked && robot.SparkUnlockId == _UnlockInfo.Id.Value) {
      _RequestTrickButton.Interactable = false;
    }
    else {
      _RequestTrickButton.Interactable = true;
    }
  }

  private void OnUpgradeClicked() {
    string hexPieceId = _UnlockInfo.UpgradeCostItemId;
    int unlockCost = _UnlockInfo.UpgradeCostAmountNeeded;

    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(hexPieceId, unlockCost)) {
      playerInventory.RemoveItemAmount(hexPieceId, unlockCost);
      UpdateInventoryLabel(hexPieceId, _FragmentInventoryLabel);

      if (_ButtonCostPaidSuccessCallback != null) {
        _ButtonCostPaidSuccessCallback(_UnlockInfo);
      }

      _UnlockUpgradeButton.Interactable = false;
      PlayUpgradeAnimation();
    }
  }

  private void PlayUpgradeAnimation() {
    _UpgradeTween = DOTween.Sequence();
    _UpgradeTween.Append(
      _UnlockableTintBackground.DOColor(UIColorPalette.GetUpgradeTintData(_UnlockInfo.CoreUpgradeTintColorName).TintColor, _UpgradeTween_sec));
    _UpgradeTween.Join(_UnlockableIcon.DOColor(Color.white, _UpgradeTween_sec));
    _UpgradeTween.AppendCallback(ResolveOnNewUnlock);
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedWin);
  }

  private void ResolveOnNewUnlock() {
    UpdateState();
    _NewUnlock = true;
  }

  private void OnSparkClicked() {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded)) {
      playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
      UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksInventoryLabel);

      RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);
      UpdateState();
    }
  }

  private void UpdateInventoryLabel(string itemId, AnkiTextLabel label) {
    ItemData itemData = ItemDataConfig.GetData(itemId);
    label.text = Localization.GetWithArgs(LocalizationKeys.kLabelColonCount, itemData.GetPluralName(), itemId);
  }

  private void HandleSparkUnlockEnded(object message) {
    UpdateState();
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.SparkUnlockEnded>(HandleSparkUnlockEnded);
    RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
    if (_UpgradeTween != null) {
      _UpgradeTween.Kill();
    }
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
    closeAnimation.AppendCallback(() => {
      if (UnlockablesManager.Instance.OnNewUnlock != null && _NewUnlock) {
        UnlockablesManager.Instance.OnNewUnlock.Invoke();
      }
    });
  }
}
