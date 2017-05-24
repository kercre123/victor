using Cozmo.UI;
using Cozmo.Challenge;
using System.Collections.Generic;
using UnityEngine;
using DataPersistence;

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
    private GameObject _LockedTextContainer;

    [SerializeField]
    private CozmoText _UnlockedText;

    [SerializeField]
    private CozmoText _SparksCountText;

    private UnlockableInfo _UnlockInfo;

    public void InitializeSparksDetailModal(UnlockableInfo unlockInfo, bool isLocked) {

      _UnlockInfo = unlockInfo;

      _Icon.sprite = unlockInfo.CoreUpgradeIcon;
      _Description.text = Localization.Get(unlockInfo.DescriptionKey);
      _Title.text = Localization.Get(unlockInfo.TitleKey);

      _LockedTextContainer.gameObject.SetActive(isLocked);
      _UnlockedText.gameObject.SetActive(!isLocked);

      _SparkButton.Initialize(() => {
        SparkCozmo(unlockInfo);
      }, "spark_cozmo_button", this.DASEventDialogName);

      _SparkButton.gameObject.SetActive(!isLocked);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);

      UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksCountText);
      UpdateState();

    }

    public void InitializeSparksDetailModal(ChallengeManager.ChallengeStatePacket challengePacket, bool isLocked) {
      _Icon.sprite = challengePacket.Data.ChallengeIcon;
      _Description.text = Localization.Get(challengePacket.Data.ChallengeDescriptionLocKey);
      _Title.text = Localization.Get(challengePacket.Data.ChallengeTitleLocKey);

      _LockedTextContainer.gameObject.SetActive(isLocked);
      _UnlockedText.gameObject.SetActive(!isLocked);

      _SparkButton.Initialize(() => {
        SparkCozmo(challengePacket);
      }, "spark_cozmo_button", this.DASEventDialogName);

      _SparkButton.gameObject.SetActive(!isLocked);
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
      playerInventory.RemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
      UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksCountText);

      DAS.Event("meta.upgrade_replay", _UnlockInfo.Id.Value.ToString(), DASUtil.FormatExtraData(_UnlockInfo.RequestTrickCostAmountNeeded.ToString()));

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
      UpdateState();
      DataPersistenceManager.Instance.Save();
    }

    private void SparkCozmo(ChallengeManager.ChallengeStatePacket challengePacket) {

    }

    private void UpdateInventoryLabel(string itemId, CozmoText label) {
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      ItemData itemData = ItemDataConfig.GetData(itemId);
      label.text = Localization.GetWithArgs(LocalizationKeys.kLabelTotalCount,
                                            itemData.GetPluralName(),
                                            Localization.GetNumber(playerInventory.GetItemAmount(itemId)));
    }

    private void UpdateState() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null && robot.IsSparked && robot.SparkUnlockId == _UnlockInfo.Id.Value) {
        _SparkButton.Interactable = false;
        _UnlockedText.text = Localization.Get(LocalizationKeys.kSparksSparked);
      }
      else {
        Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        if (UnlockablesManager.Instance.IsUnlocked(_UnlockInfo.Id.Value)) {
          _SparkButton.Interactable = playerInventory.CanRemoveItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
          if (_SparkButton.Interactable) {
            _UnlockedText.text = Localization.Get(LocalizationKeys.kSparksSparkCozmo);
          }
          else {
            _UnlockedText.text = Localization.Get(LocalizationKeys.kUnlockableBitsRequiredTitle);
          }
        }
      }
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
          UpdateState();
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
        playerInventory.AddItemAmount(_UnlockInfo.RequestTrickCostItemId, _UnlockInfo.RequestTrickCostAmountNeeded);
        UpdateInventoryLabel(_UnlockInfo.RequestTrickCostItemId, _SparksCountText);
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
