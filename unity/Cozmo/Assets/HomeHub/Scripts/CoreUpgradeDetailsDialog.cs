using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DataPersistence;
using Cozmo;
using DG.Tweening;
using System.Collections.Generic;

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

  [SerializeField]
  private GameObject _SparkImagePrefab;
  [SerializeField]
  private GameObject _DimmerPrefab;
  [SerializeField]
  private AnkiTextLabel _SparksTimerLabel;
  [SerializeField]
  private AnkiTextLabel _SparksButtonDescriptionLabel;
  [SerializeField]
  private Vector2 _SparkTargetCenter;
  [SerializeField]
  private float _SparkTargetRandOffset = 1.5f;
  [SerializeField]
  private float _SparkHoldTimeSec = 1.0f;

  private float _SparkButtonPressedTime = 0;

  private float _SparkStopTime = -1.0f;
  private CanvasGroup _DimBackgroundInstance = null;
  private Sequence _SparksSequenceTweener = null;
  private List<Transform> _SparkImageInsts = new List<Transform>();

  private UnlockableInfo _UnlockInfo;

  private CoreUpgradeRequestedHandler _ButtonCostPaidSuccessCallback;

  private Sequence _UpgradeTween;

  private bool _NewUnlock = false;

  public void Initialize(UnlockableInfo unlockInfo, CozmoUnlocksPanel.CozmoUnlockState unlockState, CoreUpgradeRequestedHandler buttonCostPaidCallback) {
    _UnlockInfo = unlockInfo;
    _ButtonCostPaidSuccessCallback = buttonCostPaidCallback;

    _UnlockUpgradeButtonContainer.gameObject.SetActive(false);
    _RequestTrickButtonContainer.gameObject.SetActive(false);
    _SparksTimerLabel.gameObject.SetActive(false);

    if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked) {
      _UnlockableIcon.color = Color.white;
      if (unlockInfo.UnlockableType == UnlockableType.Action) {
        _RequestTrickButtonContainer.gameObject.SetActive(true);
        SetupButton(_RequestTrickButton, null, "request_trick_button",
          unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded, _SparksInventoryLabel);
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.SparkUnlockEnded>(HandleSparkUnlockEnded);

        _RequestTrickButton.onPress.AddListener(OnSparkPressed);
        _RequestTrickButton.onRelease.AddListener(OnSparkReleased);
      }
    }
    else if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
      _UnlockUpgradeButtonContainer.gameObject.SetActive(true);
      SetupButton(_UnlockUpgradeButton, OnUpgradeClicked, "request_upgrade_button",
        unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded, _FragmentInventoryLabel);
      _UnlockableIcon.color = Color.gray;
    }

    _UnlockableNameLabel.text = Localization.Get(unlockInfo.TitleKey);
    _UnlockableDescriptionLabel.text = Localization.Get(unlockInfo.DescriptionKey);

    _UnlockableIcon.sprite = unlockInfo.CoreUpgradeIcon;
    _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesNeeded,
      unlockInfo.CubesRequired,
      ItemDataConfig.GetCubeData().GetAmountName(unlockInfo.CubesRequired));

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
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      _RequestTrickButton.Interactable = playerInventory.CanRemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
    }
    _SparksButtonDescriptionLabel.gameObject.SetActive(_RequestTrickButton.isActiveAndEnabled && _RequestTrickButton.Interactable);
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
    _NewUnlock = true;
    CloseView();
  }

  private void OnSparkPressed() {
    _SparkButtonPressedTime = Time.time;

    // attach a dimmer to show hold needed and an animation
    CleanUpSparkAnimations();

    if (_DimBackgroundInstance == null) {
      _DimBackgroundInstance = Instantiate(_DimmerPrefab.gameObject).GetComponent<CanvasGroup>();
      _DimBackgroundInstance.transform.SetParent(transform, false);
      _DimBackgroundInstance.blocksRaycasts = false;
    }
    _DimBackgroundInstance.alpha = 0;

    _SparksSequenceTweener = DOTween.Sequence();
    _SparksSequenceTweener.Join(_DimBackgroundInstance.DOFade(1, _SparkHoldTimeSec).SetEase(UIDefaultTransitionSettings.Instance.FadeInEasing));


    for (int i = 0; i < _UnlockInfo.UpgradeCostAmountNeeded; ++i) {
      Transform newSpark = UIManager.CreateUIElement(_SparkImagePrefab, _RequestTrickButton.transform.parent).transform;
      Vector3 endPos = new Vector3(_SparkTargetCenter.x + Random.Range(-_SparkTargetRandOffset, _SparkTargetRandOffset),
                                   _SparkTargetCenter.y + Random.Range(-_SparkTargetRandOffset, _SparkTargetRandOffset));

      RectTransform rectTransform = newSpark.GetComponent<RectTransform>();
      rectTransform.transform.position = _RequestTrickButton.transform.position;
      _SparksSequenceTweener.Join(newSpark.DOMove(endPos, _SparkHoldTimeSec)).SetEase(Ease.OutBack);
      _SparkImageInsts.Add(newSpark);
    }
  }

  private void OnSparkReleased() {
    float heldTime = (Time.time - _SparkButtonPressedTime);
    if (heldTime >= _SparkHoldTimeSec) {
      StartSparkUnlock();
      CleanUpSparkAnimations();
    }
    else {
      // Reverse the animations...
      if (_SparksSequenceTweener != null) {
        _SparksSequenceTweener.Kill();
        _SparksSequenceTweener = null;
      }
      _SparksSequenceTweener = DOTween.Sequence();
      _SparksSequenceTweener.Join(_DimBackgroundInstance.DOFade(0, heldTime));
      for (int i = 0; i < _SparkImageInsts.Count; ++i) {
        _SparksSequenceTweener.Join(_SparkImageInsts[i].DOMove(_RequestTrickButton.transform.position, heldTime));
      }
      _SparksSequenceTweener.OnComplete(HandleSparkReverseAnimationEnd);
    }
  }

  private void CleanUpSparkAnimations() {
    // DoTweenFade keeps a reference so we can't just kill and Destroy
    // the instance without a ton of exceptions, so just destroy on close.
    if (_DimBackgroundInstance != null) {
      _DimBackgroundInstance.alpha = 0;
    }

    if (_SparksSequenceTweener != null) {
      _SparksSequenceTweener.Kill();
      _SparksSequenceTweener = null;
    }

    for (int i = 0; i < _SparkImageInsts.Count; ++i) {
      Destroy(_SparkImageInsts[i].gameObject);
    }
    _SparkImageInsts.Clear();
  }

  private void HandleSparkReverseAnimationEnd() {
    CleanUpSparkAnimations();
  }

  private void Update() {
    if (_SparkStopTime >= 0 && _SparkStopTime < Time.time) {
      StopSparkUnlock();
    }
    else if (_SparkStopTime > 0) {
      _SparksTimerLabel.text = Localization.GetWithArgs(LocalizationKeys.kSparksSparked, (int)(_SparkStopTime - Time.time));
    }
  }

  private void UpdateInventoryLabel(string itemId, AnkiTextLabel label) {
    ItemData itemData = ItemDataConfig.GetData(itemId);
    label.text = Localization.GetWithArgs(LocalizationKeys.kLabelColonCount, itemData.GetPluralName(), itemId);
  }

  private void HandleSparkUnlockEnded(object message) {
    StopSparkUnlock();
  }

  private void StartSparkUnlock() {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    // Inventory valid was already checked when the button was initialized.
    playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
    UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksInventoryLabel);

    _SparkStopTime = Time.time + _UnlockInfo.TimeSparkedSec;
    _SparksTimerLabel.gameObject.SetActive(true);
    _SparksTimerLabel.text = Localization.GetWithArgs(LocalizationKeys.kSparksSparked, (int)(_SparkStopTime - Time.time));
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Spark);
    RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);

    // apparently not a bigflag in unity
    RobotEngineManager.Instance.CurrentRobot.SetFlashingBackpackLED(Anki.Cozmo.LEDId.LED_BACKPACK_FRONT, Color.blue);
    RobotEngineManager.Instance.CurrentRobot.SetFlashingBackpackLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.blue);
    RobotEngineManager.Instance.CurrentRobot.SetFlashingBackpackLED(Anki.Cozmo.LEDId.LED_BACKPACK_BACK, Color.blue);
    UpdateState();
  }
  private void StopSparkUnlock() {
    if (RobotEngineManager.Instance.CurrentRobot.IsSparked) {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
      RobotEngineManager.Instance.CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_FRONT, Color.black);
      RobotEngineManager.Instance.CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.black);
      RobotEngineManager.Instance.CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_BACK, Color.black);
      RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
    }
    _SparksTimerLabel.gameObject.SetActive(false);
    _SparkStopTime = -1.0f;
    UpdateState();
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.SparkUnlockEnded>(HandleSparkUnlockEnded);
    StopSparkUnlock();
    CleanUpSparkAnimations();
    // Because of a bug within DOTween Fades don't release even after being killed, so clean up
    if (_DimBackgroundInstance != null) {
      Destroy(_DimBackgroundInstance.gameObject);
    }
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
    // TODO: if we're moving activities out of here do we ever want to return?
    closeAnimation.AppendCallback(() => {
      if (UnlockablesManager.Instance.OnNewUnlock != null && _NewUnlock) {
        UnlockablesManager.Instance.OnNewUnlock.Invoke();
      }
    });
  }
}
