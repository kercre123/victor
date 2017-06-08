using Cozmo.Challenge;
using Cozmo.RequestGame;
using Cozmo.UI;
using DataPersistence;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksDetailModal : BaseModal {
    public delegate void SparkGameClickedHandler(string challengeId);
    public static event SparkGameClickedHandler OnSparkGameClicked;

    [SerializeField]
    private CozmoImage _Icon;

    [SerializeField]
    private CozmoText _Description;

    [SerializeField]
    private CozmoText _Title;

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

    private HasHiccupsAlertController _HasHiccupsAlertController;

    private bool _IsSparkingGame = false;

    protected override void CleanUp() {
      base.CleanUp();
      RequestGameManager.Instance.EnableRequestGameBehaviorGroups();
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);
      StopSparkTrick(isCleanup: true);
      if (_QuitConfirmAlertModal != null) {
        UIManager.CloseModalImmediately(_QuitConfirmAlertModal);
      }
      if (_HasHiccupsAlertController != null) {
        _HasHiccupsAlertController.Cleanup();
      }
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

    private void OnApplicationPause(bool pauseStatus) {
      DAS.Debug("CoreUpgradeDetailsDialog.OnApplicationPause", "Application pause: " + pauseStatus);
      if (pauseStatus) {
        StopSparkTrick();
      }
    }

    private void OnDestroy() {
      CleanUpButtonState();
    }

    private void InitializeDescription(Sprite icon, string titleLocKey, string descLocKey) {
      _Icon.sprite = icon;
      _Title.text = Localization.Get(titleLocKey);
      _Description.text = Localization.Get(descLocKey);
    }

    private void InitializeUnlockInfo() {
      this.DASEventDialogName = this.DASEventDialogName + "_" + _UnlockInfo.DASName;
      _SparksCostText.text = Localization.GetNumber(_UnlockInfo.RequestTrickCostAmount);
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

      // Handle edge cases
      _HasHiccupsAlertController = new HasHiccupsAlertController();
      RequestGameManager.Instance.DisableRequestGameBehaviorGroups();
    }

    private void SparkCozmo(ChallengeManager.ChallengeStatePacket challengePacket) {
      if (ShowEdgeCaseAlertIfNeeded(requireCubes: true)) {
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

    public void InitializeSparksDetailModal(UnlockableInfo unlockInfo) {
      _UnlockInfo = unlockInfo;
      InitializeDescription(unlockInfo.CoreUpgradeIcon, unlockInfo.TitleKey, unlockInfo.DescriptionKey);
      InitializeUnlockInfo();

      _SparkButton.Initialize(() => {
        SparkCozmo(unlockInfo);
      }, "spark_specific_trick_button", this.DASEventDialogName);
      InitializeButtonState();

      // Handle edge cases
      _HasHiccupsAlertController = new HasHiccupsAlertController();
      RequestGameManager.Instance.DisableRequestGameBehaviorGroups();

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);
    }

    private void SparkCozmo(UnlockableInfo unlockInfo) {
      if (ShowEdgeCaseAlertIfNeeded(requireCubes: false)) {
        return;
      }

      if (UnlockablesManager.Instance.OnSparkStarted != null) {
        UnlockablesManager.Instance.OnSparkStarted.Invoke(_UnlockInfo.Id.Value);
      }

      RemoveCostFromInventory();

      DAS.Event("meta.upgrade_replay", _UnlockInfo.Id.Value.ToString(),
                DASUtil.FormatExtraData(_UnlockInfo.RequestTrickCostAmount.ToString()));

      // Post sparked audio SFX
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Spark_Launch);

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        // Give Sparked Behavior music ownership        
        Anki.AudioMetaData.SwitchState.Sparked sparkedMusicState = _UnlockInfo.SparkedMusicState.Sparked;
        if (sparkedMusicState == Anki.AudioMetaData.SwitchState.Sparked.Invalid) {
          sparkedMusicState = SparkedMusicStateWrapper.DefaultState().Sparked;
        }
        RobotEngineManager.Instance.CurrentRobot.SetSparkedMusicState(sparkedMusicState);

        RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);
      }

      UpdateButtonState();
    }

    private void StopSparkTrick(bool isCleanup = false) {
      // Send stop message to engine
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        if (RobotEngineManager.Instance.CurrentRobot.IsSparked) {
          RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
        }

        // Take the audio state back to freeplay
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Freeplay);

        if (!isCleanup) {
          UpdateButtonState();
        }
      }
    }

    private void HandleSparkEnded(Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine message) {
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
        // Cozmo failed to perform the spark
        Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.AddItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmount);
      }

      StopSparkTrick();
      if (_QuitConfirmAlertModal != null) {
        UIManager.CloseModal(_QuitConfirmAlertModal);
      }

      DataPersistenceManager.Instance.Save();
    }

    #endregion

    #region Edge Cases

    private bool ShowEdgeCaseAlertIfNeeded(bool requireCubes) {
      bool edgeCaseFound = true;
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        int currentNumCubes = robot.LightCubes.Count;
        if (requireCubes && currentNumCubes < _UnlockInfo.CubesRequired) {
          NeedCubesAlertHelper.OpenNeedCubesAlert(currentNumCubes,
                                                  _UnlockInfo.CubesRequired,
                                                  Localization.Get(_ChallengeTitleLocKey),
                                                  this.PriorityData);
        }
        else if (robot.CurrentBehaviorClass == Anki.Cozmo.BehaviorClass.DriveOffCharger) {
          OpenCozmoNotReadyAlert();
        }
        else if (robot.CurrentBehaviorClass == Anki.Cozmo.BehaviorClass.ReactToRobotShaken) {
          OpenCozmoDizzyAlert();
        }
        else if (robot.TreadState != Anki.Cozmo.OffTreadsState.OnTreads) {
          OpenCozmoNotOnTreadsAlert();
        }
        else if (robot.HasHiccups) {
          _HasHiccupsAlertController.OpenCozmoHasHiccupsAlert(this.PriorityData);
        }
        else {
          edgeCaseFound = false;
        }
      }

      return edgeCaseFound;
    }

    // Cozmo isn't done driving off the charger.
    private void OpenCozmoNotReadyAlert() {
      var cozmoNotReadyData = new AlertModalData("cozmo_driving_off_charger_alert",
                                LocalizationKeys.kChallengeDetailsCozmoIsStillWakingUpModalTitle,
                                LocalizationKeys.kChallengeDetailsCozmoIsStillWakingUpModalDescription,
                                new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                timeoutSec: 10.0f);

      UIManager.OpenAlert(cozmoNotReadyData, ModalPriorityData.CreateSlightlyHigherData(this.PriorityData));
    }

    private void OpenCozmoDizzyAlert() {
      var cozmoDizzyData = new AlertModalData("cozmo_dizzy_alert",
                             LocalizationKeys.kChallengeDetailsCozmoDizzyTitle,
                             LocalizationKeys.kChallengeDetailsCozmoDizzyDescription,
                             new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      UIManager.OpenAlert(cozmoDizzyData, ModalPriorityData.CreateSlightlyHigherData(this.PriorityData));
    }

    private void OpenCozmoNotOnTreadsAlert() {
      var cozmoNotOnTreadsData = new AlertModalData("cozmo_off_treads_alert",
                                   LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsTitle,
                                   LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsDescription,
                                   new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      UIManager.OpenAlert(cozmoNotOnTreadsData, ModalPriorityData.CreateSlightlyHigherData(this.PriorityData));
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
      var staySparkedButtonData = new AlertModalButtonData("stay_sparked_button", LocalizationKeys.kButtonStaySparked, HandleStaySparked,
               Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.AudioMetaData.GameEvent.Ui.Click_Back));
      var leaveSparkButtonData = new AlertModalButtonData("quit_spark_confirm_button", LocalizationKeys.kButtonLeave, HandleLeaveSpark);

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

      UpdateButtonState();
    }

    private void CleanUpButtonState() {
      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      playerInventory.ItemCountSet -= HandleItemValueChanged;
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
