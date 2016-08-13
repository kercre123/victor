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
  private GameObject _AvailablePromptContainer;

  [SerializeField]
  private GameObject _AffordableHighlightContainer;

  [SerializeField]
  private AnkiTextLabel _AvailablePromptLabel;

  [SerializeField]
  private AnkiTextLabel _AvailablePromptCost;

  [SerializeField]
  private Text _ButtonCounter;


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
  private float _UpgradeTween_sec = 0.6f;

  [SerializeField]
  private GameObject _SparkImagePrefab;
  [SerializeField]
  private GameObject _DimmerPrefab;
  [SerializeField]
  private AnkiTextLabel _SparksButtonDescriptionLabel;
  [SerializeField]
  private Vector2 _SparkTargetCenter;
  [SerializeField]
  private float _SparkTargetRandOffset = 1.5f;
  [SerializeField]
  private float _SparkHoldTimeSec = 1.0f;

  private float _SparkButtonPressedTime = -1.0f;

  private float _SparkStopTime = -1.0f;
  private CanvasGroup _DimBackgroundInstance = null;
  private Sequence _SparksSequenceTweener = null;
  private List<Transform> _SparkImageInsts = new List<Transform>();

  private UnlockableInfo _UnlockInfo;

  private CoreUpgradeRequestedHandler _ButtonCostPaidSuccessCallback;

  private Sequence _UpgradeTween;

  public void Initialize(UnlockableInfo unlockInfo, CozmoUnlocksPanel.CozmoUnlockState unlockState, CoreUpgradeRequestedHandler buttonCostPaidCallback) {
    _UnlockInfo = unlockInfo;
    _ButtonCostPaidSuccessCallback = buttonCostPaidCallback;

    _UnlockUpgradeButtonContainer.gameObject.SetActive(false);
    _RequestTrickButtonContainer.gameObject.SetActive(false);
    _AvailablePromptContainer.SetActive(false);
    _AffordableHighlightContainer.gameObject.SetActive(false);
    // QA testing jumps around during stages. As a failsafe just give the amount needed since they can't exist.
    Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

    // No quitting until they've finished onboarding
    if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Upgrades)) {
      if (_UnlockInfo.UnlockableType == UnlockableType.Action) {
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
        SetupButton(_RequestTrickButton, null, "request_trick_button",
                    unlockInfo.RequestTrickCostItemId, unlockInfo.RequestTrickCostAmountNeeded, _SparksInventoryLabel, true);
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.SparkUnlockEnded>(HandleSparkUnlockEnded);

        _RequestTrickButton.onPress.AddListener(OnSparkPressed);
        _RequestTrickButton.onRelease.AddListener(OnSparkReleased);
      }
    }
    else if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
      _UnlockUpgradeButtonContainer.gameObject.SetActive(true);
      _FragmentInventoryContainer.gameObject.SetActive(true);
      _SparksInventoryContainer.gameObject.SetActive(false);
      _AvailablePromptContainer.SetActive(true);
      _AffordableHighlightContainer.gameObject.SetActive(true);
      _AvailablePromptLabel.gameObject.SetActive(playerInventory.CanRemoveItemAmount(unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded));
      _AvailablePromptLabel.text = Localization.Get(LocalizationKeys.kUnlockableAvailable);
      ItemData itemData = ItemDataConfig.GetData(unlockInfo.UpgradeCostItemId);
      int cost = unlockInfo.UpgradeCostAmountNeeded;
      string costName = Localization.Get(itemData.GetPluralName());
      _AvailablePromptCost.text = Localization.GetWithArgs(LocalizationKeys.kUnlockableBitsRequiredDescription, new object[] { cost, costName });
      SetupButton(_UnlockUpgradeButton, OnUpgradeClicked, "request_upgrade_button",
                  unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded, _FragmentInventoryLabel, false);
      _UnlockableIcon.color = Color.gray;
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
      button.Text = Localization.Get(LocalizationKeys.kSparksSpark);
    }
    else {
      button.Text = Localization.Get(LocalizationKeys.kUnlockableUnlock);
    }
    _ButtonCounter.text = string.Format("{0}", costAmount);
    inventoryLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelTotalCount,
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
    _ButtonCounter.text = string.Format("{0}", _UnlockInfo.RequestTrickCostAmountNeeded);
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
    // Update has already happened and processed the spark
    if (_SparkButtonPressedTime > 0) {
      // Reverse, positive case is handled by Update
      float heldTime = (Time.time - _SparkButtonPressedTime);
      if (heldTime < _SparkHoldTimeSec) {
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
      _SparkButtonPressedTime = -1.0f;
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
    if (_SparkButtonPressedTime > 0) {
      float heldTime = (Time.time - _SparkButtonPressedTime);
      if (heldTime >= _SparkHoldTimeSec) {
        StartSparkUnlock();
        CleanUpSparkAnimations();
        _SparkButtonPressedTime = -1.0f;
      }
    }
    if (_SparkStopTime >= 0 && _SparkStopTime < Time.time) {
      StopSparkUnlock();
    }
  }

  private void UpdateInventoryLabel(string itemId, AnkiTextLabel label) {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    ItemData itemData = ItemDataConfig.GetData(itemId);
    label.text = Localization.GetWithArgs(LocalizationKeys.kLabelTotalCount,
                                          itemData.GetPluralName(),
                                          playerInventory.GetItemAmount(itemId));
  }

  private void HandleSparkUnlockEnded(object message) {
    StopSparkUnlock();
  }

  private void StartSparkUnlock() {
    if (UnlockablesManager.Instance.OnSparkStarted != null) {
      UnlockablesManager.Instance.OnSparkStarted.Invoke(_UnlockInfo.Id.Value);
    }
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    TimelineEntryData sess = DataPersistenceManager.Instance.CurrentSession;
    if (sess.SparkCount.ContainsKey(_UnlockInfo.Id.Value)) {
      sess.SparkCount[_UnlockInfo.Id.Value]++;
    }
    else {
      sess.SparkCount.Add(_UnlockInfo.Id.Value, 1);
    }
    // Inventory valid was already checked when the button was initialized.
    playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
    UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksInventoryLabel);

    _SparkStopTime = Time.time + _UnlockInfo.TimeSparkedSec;
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Spark);
    RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);

    // apparently not a bitflag in unity
    RobotEngineManager.Instance.CurrentRobot.SetFlashingBackpackLED(Anki.Cozmo.LEDId.LED_BACKPACK_FRONT, Color.blue);
    RobotEngineManager.Instance.CurrentRobot.SetFlashingBackpackLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.blue);
    RobotEngineManager.Instance.CurrentRobot.SetFlashingBackpackLED(Anki.Cozmo.LEDId.LED_BACKPACK_BACK, Color.blue);
    UpdateState();
    DataPersistenceManager.Instance.Save();
  }
  private void StopSparkUnlock() {
    if (RobotEngineManager.Instance.CurrentRobot.IsSparked) {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
      RobotEngineManager.Instance.CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_FRONT, Color.black);
      RobotEngineManager.Instance.CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.black);
      RobotEngineManager.Instance.CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_BACK, Color.black);
      RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
    }
    _SparkStopTime = -1.0f;
    UpdateState();

    if (UnlockablesManager.Instance.OnSparkComplete != null) {
      UnlockablesManager.Instance.OnSparkComplete.Invoke(this);
    }
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
  }
}
