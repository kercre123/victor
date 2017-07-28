using System;
using Anki.Cozmo;
using Cozmo.Challenge;
using Cozmo.RequestGame;
using Cozmo.UI;
using DataPersistence;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksDetailModal : BaseModal {
    public delegate void SparkGameClickedHandler(string challengeId);
    public static event SparkGameClickedHandler OnSparkGameClicked;

    public delegate void SparkTrickHandler();
    public static event SparkTrickHandler OnSparkTrickStarted;
    public static event SparkTrickHandler OnSparkTrickEnded;
    public static event SparkTrickHandler OnSparkTrickQuit;

    public Action OnSparkCompleteToReturn;

    [SerializeField]
    private CozmoImage _Icon;

    [SerializeField]
    private CozmoText _Description;

    [SerializeField]
    private CozmoText _Title;

    [SerializeField]
    private CozmoText _NotSparkableLabel;

    [SerializeField]
    private GameObject[] _SparkableInfoContainers;

    [SerializeField]
    private CozmoButton _SparkButton;

    [SerializeField]
    private CozmoText _SparksCostText;

    [SerializeField]
    private CozmoText _CubesRequiredLabel;

    [SerializeField]
    private CozmoText _ButtonPromptTitle;

    [SerializeField]
    private CozmoText _ButtonPromptDescription;

    [SerializeField]
    private GameObject _SparkSpinnerContainer;

    [SerializeField]
    protected MoveTweenSettings _StartGameMoveTweenSettings;

    private UnlockableInfo _UnlockInfo;
    private string _ChallengeId;
    private string _ChallengeTitleLocKey;

    private bool _ConfirmedQuitTrick = false;
    private AlertModal _QuitConfirmAlertModal;

    private ChallengeStartEdgeCaseAlertController _EdgeCaseAlertController;

    private bool _IsSparkingGame = false;
    private bool _IsEngineDrivenTrick = false;
    private bool _DoEngineCleanUp = true;

    protected override void CleanUp() {
      base.CleanUp();

      // Skip engine cleanup if we are just about to open a second copy of SparksDetailModal (used when
      // VC is triggered for Do A Trick while a trick is open
      if (_DoEngineCleanUp) {
        RequestGameManager.Instance.EnableRequestGameBehaviorGroups();
      }
      StopSparkTrick(isDialogCleanup: true, doEngineCleanup: _DoEngineCleanUp);

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);
      if (_QuitConfirmAlertModal != null) {
        UIManager.CloseModalImmediately(_QuitConfirmAlertModal);
      }
      if (_EdgeCaseAlertController != null) {
        _EdgeCaseAlertController.CleanUp();
      }
      CleanUpButtonState();
    }

    protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      if (_MoveTweenSettings.targets.Length > 0) {
        settings.ConstructCloseMoveTween(ref closeAnimation, _MoveTweenSettings);
      }
      if (_IsSparkingGame) {
        if (_StartGameMoveTweenSettings.targets.Length > 0) {
          settings.ConstructCloseMoveTween(ref closeAnimation, _StartGameMoveTweenSettings);
        }
      }
      else {
        if (_FadeTweenSettings.targets.Length > 0) {
          settings.ConstructCloseFadeTween(ref closeAnimation, _FadeTweenSettings);
        }
      }
    }

    public void CloseAndSkipEngineCleanup() {
      // Avoid music and spark state with the robot conflicts that can occur when saying VC
      // to do a trick while this dialog is open
      _DoEngineCleanUp = false;
      CloseDialog();
    }

    private void OnApplicationPause(bool pauseStatus) {
      DAS.Debug("CoreUpgradeDetailsDialog.OnApplicationPause", "Application pause: " + pauseStatus);
      if (pauseStatus) {
        StopSparkTrick(isDialogCleanup: false, doEngineCleanup: true);
      }
    }

    private void InitializeDescription(Sprite icon, string titleLocKey, string descLocKey) {
      _Icon.sprite = icon;
      _Title.text = Localization.Get(titleLocKey);
      _Description.text = Localization.Get(descLocKey);
    }

    private void InitializeUnlockInfo() {
      this.DASEventDialogName = this.DASEventDialogName + "_" + _UnlockInfo.DASName;
      int sparkCost = _IsEngineDrivenTrick ? (int)EnumConcept.GetSparkCosts(SparkableThings.DoATrick, 0)
                                                      : _UnlockInfo.RequestTrickCostAmount;
      _SparksCostText.text = Localization.GetNumber(sparkCost);
      _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesNeeded,
        _UnlockInfo.CubesRequired,
        ItemDataConfig.GetCubeData().GetAmountName(_UnlockInfo.CubesRequired));
    }

    private void RemoveCostFromInventory() {
      // Inventory valid was already checked when the button was initialized.
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmount);
      DataPersistenceManager.Instance.Save();
    }

    #region Spark Game

    public void InitializeSparksDetailModal(ChallengeManager.ChallengeStatePacket challengePacket) {
      _UnlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(challengePacket.Data.UnlockId.Value);
      ChallengeData data = challengePacket.Data;
      _ChallengeId = data.ChallengeID;
      _ChallengeTitleLocKey = data.ChallengeTitleLocKey;
      InitializeDescription(data.ChallengeIcon, data.ChallengeTitleLocKey, data.ChallengeDescriptionLocKey);
      InitializeUnlockInfo();

      _SparkButton.Initialize(() => {
        SparkCozmo(challengePacket);
      }, "spark_specific_game_button", this.DASEventDialogName);
      InitializeButtonState();
      _NotSparkableLabel.gameObject.SetActive(false);

      // Handle edge cases      
      ChallengeEdgeCases challengeEdgeCases = ChallengeEdgeCases.CheckForDizzy | ChallengeEdgeCases.CheckForCubes
                                                                | ChallengeEdgeCases.CheckForHiccups
                                                                | ChallengeEdgeCases.CheckForDriveOffCharger
                                                                | ChallengeEdgeCases.CheckForOnTreads
                                                                | ChallengeEdgeCases.CheckForOS;
      _EdgeCaseAlertController = new ChallengeStartEdgeCaseAlertController(this.PriorityData, challengeEdgeCases);
      RequestGameManager.Instance.DisableRequestGameBehaviorGroups();
    }

    private void SparkCozmo(ChallengeManager.ChallengeStatePacket challengePacket) {
      if (ShowEdgeCaseAlertIfNeeded()) {
        return;
      }

      DAS.Event("meta.spark_specific_game", _UnlockInfo.Id.Value.ToString(),
                DASUtil.FormatExtraData(_UnlockInfo.RequestTrickCostAmount.ToString()));

      RemoveCostFromInventory();

      if (OnSparkGameClicked != null) {
        OnSparkGameClicked(_ChallengeId);
      }

      _IsSparkingGame = true;
      this.CloseDialog();
    }

    #endregion

    #region Spark Trick

    public void InitializeSparksDetailModal(UnlockableInfo unlockInfo, bool isEngineDriven) {
      _UnlockInfo = unlockInfo;
      _IsEngineDrivenTrick = isEngineDriven;

      InitializeDescription(unlockInfo.CoreUpgradeIcon, unlockInfo.TitleKey, unlockInfo.DescriptionKey);
      InitializeUnlockInfo();

      // Initialize the button to avoid an error
      _SparkButton.Initialize(() => {
        SparkCozmo(unlockInfo);
      }, "spark_specific_trick_button", this.DASEventDialogName);

      if (unlockInfo.IsSparkable) {
        InitializeButtonState();
      }

      for (int i = 0; i < _SparkableInfoContainers.Length; i++) {
        _SparkableInfoContainers[i].gameObject.SetActive(unlockInfo.IsSparkable);
      }
      _NotSparkableLabel.gameObject.SetActive(!unlockInfo.IsSparkable);

      // Handle edge cases; don't check for cubes
      ChallengeEdgeCases challengeEdgeCases = ChallengeEdgeCases.CheckForDizzy
                                    | ChallengeEdgeCases.CheckForHiccups
                                    | ChallengeEdgeCases.CheckForDriveOffCharger
                                    | ChallengeEdgeCases.CheckForOnTreads
                                    | ChallengeEdgeCases.CheckForOS;
      _EdgeCaseAlertController = new ChallengeStartEdgeCaseAlertController(this.PriorityData, challengeEdgeCases);
      RequestGameManager.Instance.DisableRequestGameBehaviorGroups();

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);

      if (isEngineDriven) {
        PlaySparkedSounds();
        // Button state already updated by InitializeButtonState above

        if (OnSparkTrickStarted != null) {
          OnSparkTrickStarted();
        }
      }
    }

    private void SparkCozmo(UnlockableInfo unlockInfo) {
      if (ShowEdgeCaseAlertIfNeeded()) {
        return;
      }

      RemoveCostFromInventory();

      DAS.Event("meta.upgrade_replay", _UnlockInfo.Id.Value.ToString(),
                DASUtil.FormatExtraData(_UnlockInfo.RequestTrickCostAmount.ToString()));

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);
      }

      PlaySparkedSounds();
      UpdateButtonState();

      if (OnSparkTrickStarted != null) {
        OnSparkTrickStarted();
      }
    }

    private void PlaySparkedSounds() {
      // Post sparked audio SFX
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Spark_Launch);
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        // Give Sparked Behavior music ownership        
        Anki.AudioMetaData.SwitchState.Sparked sparkedMusicState = _UnlockInfo.SparkedMusicState.Sparked;
        if (sparkedMusicState == Anki.AudioMetaData.SwitchState.Sparked.Invalid) {
          sparkedMusicState = SparkedMusicStateWrapper.DefaultState().Sparked;
        }
        RobotEngineManager.Instance.CurrentRobot.SetSparkedMusicState(sparkedMusicState);
      }
    }

    private void HandleSparkEnded(Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine message) {
      bool returnToHub = true;

      // Update inventory and spark cost appropriately
      if (message.success) {
        // cozmo performed the spark
        TimelineEntryData sess = DataPersistenceManager.Instance.CurrentSession;

        if (sess.SparkCount.ContainsKey(_UnlockInfo.Id.Value)) {
          sess.SparkCount[_UnlockInfo.Id.Value]++;
        }
        else {
          sess.SparkCount.Add(_UnlockInfo.Id.Value, 1);
        }
      }
      else {
        // Don't return sparks if the spark was due to a VC or the offer flow
        if (!_IsEngineDrivenTrick) {
          // Cozmo failed to perform the spark
          Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
          playerInventory.AddItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmount);
        }
        returnToHub = false;
      }

      StopSparkTrick(isDialogCleanup: false, doEngineCleanup: true);
      if (OnSparkTrickEnded != null) {
        OnSparkTrickEnded();
      }

      if (_QuitConfirmAlertModal != null) {
        UIManager.CloseModal(_QuitConfirmAlertModal);
      }

      DataPersistenceManager.Instance.Save();

      if (returnToHub) {
        CloseDialog();
        if (OnSparkCompleteToReturn != null) {
          OnSparkCompleteToReturn();
        }
      }
      else if (_IsEngineDrivenTrick) {
        CloseDialog();
      }
    }

    private void StopSparkTrick(bool isDialogCleanup, bool doEngineCleanup) {
      if (doEngineCleanup) {
        // Send stop message to engine
        if (RobotEngineManager.Instance.CurrentRobot != null
            && RobotEngineManager.Instance.CurrentRobot.IsSparked) {
          RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
        }

        // Take the audio state back to freeplay
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Freeplay);
      }

      if (!isDialogCleanup) {
        UpdateButtonState();
      }
    }

    #endregion

    #region Edge Cases

    private bool ShowEdgeCaseAlertIfNeeded() {
      return _EdgeCaseAlertController.ShowEdgeCaseAlertIfNeeded(_ChallengeTitleLocKey,
                                                                _UnlockInfo.CubesRequired,
                                                                UnlockablesManager.Instance.IsOSSupported(_UnlockInfo),
                                                                _UnlockInfo.AndroidReleaseVersion);
    }

    #endregion

    #region Confirm Quit Spark Trick

    protected override void HandleUserClose() {
      if (RobotEngineManager.Instance.CurrentRobot != null && RobotEngineManager.Instance.CurrentRobot.IsSparked) {
        CreateConfirmQuitTrickAlert();
      }
      else {
        base.HandleUserClose();
      }
    }

    private void CreateConfirmQuitTrickAlert() {
      // Hook up callbacks
      var staySparkedButtonData = new AlertModalButtonData("stay_sparked_button", LocalizationKeys.kButtonStaySparked,
                                                           HandleStaySparked,
               Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.AudioMetaData.GameEvent.Ui.Click_Back));
      var leaveSparkButtonData = new AlertModalButtonData("quit_spark_confirm_button", LocalizationKeys.kButtonLeave,
                                                          HandleLeaveSpark);

      var confirmQuitSparkData = new AlertModalData("confirm_quit_spark_alert",
            LocalizationKeys.kSparksSparkConfirmQuit,
            LocalizationKeys.kSparksSparkConfirmQuitDescription,
            staySparkedButtonData,
            leaveSparkButtonData,
            dialogCloseAnimationFinishedCallback: HandleQuitTrickAlertClosed);

      var confirmQuitPriorityData = ModalPriorityData.CreateSlightlyHigherData(this.PriorityData);

      // Open confirmation dialog instead
      UIManager.OpenAlert(confirmQuitSparkData, confirmQuitPriorityData, HandleConfirmQuitTrickAlertCreated);
      _ConfirmedQuitTrick = false;
    }

    private void HandleConfirmQuitTrickAlertCreated(AlertModal newAlertModal) {
      _QuitConfirmAlertModal = newAlertModal;
    }

    private void HandleQuitTrickAlertClosed() {
      if (_ConfirmedQuitTrick) {
        if (OnSparkTrickQuit != null) {
          OnSparkTrickQuit();
        }
        CloseDialog();
      }
      _ConfirmedQuitTrick = false;
    }

    private void HandleStaySparked() {
      _ConfirmedQuitTrick = false;
    }

    private void HandleLeaveSpark() {
      _ConfirmedQuitTrick = true;
    }

    #endregion

    #region UI Updates

    private void InitializeButtonState() {
      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      playerInventory.ItemCountSet += HandleItemValueChanged;
      playerInventory.ItemCountUpdated += HandleItemValueChanged;

      UpdateButtonState();
    }

    private void CleanUpButtonState() {
      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      playerInventory.ItemCountSet -= HandleItemValueChanged;
      playerInventory.ItemCountUpdated -= HandleItemValueChanged;
    }

    private void HandleItemValueChanged(string itemId, int delta, int newCount) {
      if (itemId == _UnlockInfo.RequestTrickCostItemId) {
        UpdateButtonState();
      }
    }

    private void UpdateButtonState() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null && robot.IsSparked && robot.SparkUnlockId == _UnlockInfo.Id.Value) {
        _SparkButton.Interactable = false;
        _SparksCostText.color = UIColorPalette.ButtonSparkTintColor.CannotAffordColor;
        _ButtonPromptTitle.text = Localization.Get(LocalizationKeys.kSparksSparked);
        _ButtonPromptDescription.text = Localization.Get(_UnlockInfo.SparkedStateDescription);
        _SparkSpinnerContainer.gameObject.SetActive(true);
      }
      else {
        Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        if (playerInventory.CanRemoveItemAmount(_UnlockInfo.RequestTrickCostItemId,
                                                _UnlockInfo.RequestTrickCostAmount)) {
          _SparkButton.Interactable = true;
          _SparksCostText.color = UIColorPalette.ButtonSparkTintColor.CanAffordColor;
          _ButtonPromptTitle.text = Localization.Get(LocalizationKeys.kSparksSparkCozmo);
          _ButtonPromptDescription.text = Localization.Get(_UnlockInfo.SparkButtonDescription);
        }
        else {
          _SparkButton.Interactable = false;
          _SparksCostText.color = UIColorPalette.ButtonSparkTintColor.CannotAffordColor;
          _ButtonPromptTitle.text = Localization.Get(LocalizationKeys.kSparksNotEnoughSparksTitle);
          _ButtonPromptDescription.text = Localization.Get(LocalizationKeys.kSparksNotEnoughSparksDesc);
        }
        _SparkSpinnerContainer.gameObject.SetActive(false);
      }

      // Force the text to update
      _ButtonPromptTitle.enabled = false;
      _ButtonPromptTitle.enabled = true;
      _ButtonPromptDescription.enabled = false;
      _ButtonPromptDescription.enabled = true;
    }

    #endregion
  }
}
