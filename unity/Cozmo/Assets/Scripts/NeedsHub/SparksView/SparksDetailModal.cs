using Cozmo.UI;
using Cozmo.Challenge;
using DataPersistence;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksDetailModal : BaseModal {

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

    private UnlockableInfo _UnlockInfo;

    public void InitializeSparksDetailModal(UnlockableInfo unlockInfo) {
      _UnlockInfo = unlockInfo;

      _Icon.sprite = unlockInfo.CoreUpgradeIcon;
      _Description.text = Localization.Get(unlockInfo.DescriptionKey);
      _Title.text = Localization.Get(unlockInfo.TitleKey);

      // Require a flat rate even if a spark is requested multiple times
      _SparksCostText.text = Localization.GetNumber(unlockInfo.RequestTrickCostAmountNeededMin);

      _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesNeeded,
        unlockInfo.CubesRequired,
        ItemDataConfig.GetCubeData().GetAmountName(unlockInfo.CubesRequired));

      _SparkButton.Initialize(() => {
        SparkCozmo(unlockInfo);
      }, "spark_cozmo_button", this.DASEventDialogName);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);

      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      playerInventory.ItemCountSet += HandleItemValueChanged;

      UpdateButtonState();
    }

    public void InitializeSparksDetailModal(ChallengeManager.ChallengeStatePacket challengePacket) {
      _Icon.sprite = challengePacket.Data.ChallengeIcon;
      _Description.text = Localization.Get(challengePacket.Data.ChallengeDescriptionLocKey);
      _Title.text = Localization.Get(challengePacket.Data.ChallengeTitleLocKey);

      _SparkButton.Initialize(() => {
        SparkCozmo(challengePacket);
      }, "spark_cozmo_button", this.DASEventDialogName);
    }

    private void OnDestroy() {
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

    private void SparkCozmo(UnlockableInfo unlockInfo) {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (robot.HasHiccups) {
          // TODO: robot has hiccups modal and early out
        }
      }

      if (UnlockablesManager.Instance.OnSparkStarted != null) {
        UnlockablesManager.Instance.OnSparkStarted.Invoke(_UnlockInfo.Id.Value);
      }
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

      // Inventory valid was already checked when the button was initialized.
      playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeededMin);

      DAS.Event("meta.upgrade_replay", _UnlockInfo.Id.Value.ToString(),
                DASUtil.FormatExtraData(_UnlockInfo.RequestTrickCostAmountNeededMin.ToString()));

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
      DataPersistenceManager.Instance.Save();
    }

    private void SparkCozmo(ChallengeManager.ChallengeStatePacket challengePacket) {

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
                                                _UnlockInfo.RequestTrickCostAmountNeededMin)) {
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

    private void OnApplicationPause(bool pauseStatus) {
      DAS.Debug("CoreUpgradeDetailsDialog.OnApplicationPause", "Application pause: " + pauseStatus);
      if (pauseStatus) {
        StopSparkUnlock();
      }
    }

    private void StopSparkUnlock(bool isCleanup = false) {
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
        playerInventory.AddItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeededMin);
      }

      StopSparkUnlock();
      DataPersistenceManager.Instance.Save();
    }

    protected override void CleanUp() {
      base.CleanUp();
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);
      StopSparkUnlock(isCleanup: true);
    }
  }
}
