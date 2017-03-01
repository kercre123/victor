using Anki.Cozmo.ExternalInterface;
using Anki.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsVersionsPanel : MonoBehaviour {

    [SerializeField]
    private AnkiTextLegacy _AppVersionLabel;

    [SerializeField]
    private AnkiTextLegacy _CozmoVersionLabel;

    [SerializeField]
    private AnkiTextLegacy _SerialNumberLabel;

    [SerializeField]
    private AnkiTextLegacy _DeviceIDLabel;

    [SerializeField]
    private AnkiTextLegacy _AppRunLabel;

    [SerializeField]
    private CozmoButtonLegacy _SupportButton;

    [SerializeField]
    private SettingsSupportInfoModal _SupportInfoModalPrefab;

    private SettingsSupportInfoModal _SupportInfoModalInstance;

    [SerializeField]
    private CozmoButtonLegacy _EraseCozmoButton;

    [SerializeField]
    private float _SecondsToConfirmEraseCozmo = 5f;

    private LongPressConfirmationModal _EraseCozmoModalInstance;

    [SerializeField]
    private float _SecondsToConfirmRestoreCozmo = 5f;

    private LongPressConfirmationModal _RestoreCozmoModalInstance;

    private bool _RestoreButtonIsActive = true;

    private ModalPriorityData _SettingsModalPriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                                                 LowPriorityModalAction.CancelSelf,
                                                                                 HighPriorityModalAction.Stack);

    private void Awake() {
      string dasEventViewName = "settings_version_panel";

      _SupportButton.Initialize(HandleOpenSupportViewButtonTapped, "support_button", dasEventViewName);
      _EraseCozmoButton.Initialize(HandleOpenEraseCozmoViewButtonTapped, "open_erase_cozmo_view_button", dasEventViewName);
    }

    private void Start() {
      RobotEngineManager.Instance.AddCallback<DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.AddCallback<RestoreRobotOptions>(HandleRestoreRobotOptions);
      RobotEngineManager.Instance.SendRequestDeviceData();

      // Fill out Cozmo version args
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      _CozmoVersionLabel.FormattingArgs = new object[] { ShortenData(robot.FirmwareVersion.ToString()) };

      // Fill out Serial number args
      _SerialNumberLabel.FormattingArgs = new object[] { robot.SerialNumber.ToString("X8") };

      robot.RequestRobotRestoreData();
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleEraseRobotStatus);

      if (_SupportInfoModalInstance != null) {
        _SupportInfoModalInstance.CloseDialogImmediately();
        _SupportInfoModalInstance.OnOpenRestoreCozmoViewButtonTapped -= HandleOpenRestoreCozmoViewButtonTapped;
      }
      if (_EraseCozmoModalInstance != null) {
        _EraseCozmoModalInstance.CloseDialogImmediately();
      }

      RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);
      if (_RestoreCozmoModalInstance != null) {
        _RestoreCozmoModalInstance.CloseDialogImmediately();
      }
    }

    private string ShortenData(string data) {
      string shortData = data;
      int desiredStringLength = DefaultSettingsValuesConfig.Instance.CharactersOfAppInfoToShow;
      if (shortData.Length > desiredStringLength) {
        shortData = shortData.Substring(0, desiredStringLength);
      }
      return shortData;
    }

    private void HandleDeviceDataMessage(DeviceDataMessage message) {
      for (int i = 0; i < message.dataList.Length; ++i) {
        Anki.Cozmo.DeviceDataPair currentPair = message.dataList[i];
        string shortDataValue = ShortenData(currentPair.dataValue);
        switch (currentPair.dataType) {
        case Anki.Cozmo.DeviceDataType.DeviceID: {
            _DeviceIDLabel.FormattingArgs = new object[] { shortDataValue };
            break;
          }
        case Anki.Cozmo.DeviceDataType.AppRunID: {
            _AppRunLabel.FormattingArgs = new object[] { shortDataValue };
            break;
          }
        case Anki.Cozmo.DeviceDataType.LastAppRunID: {
            // Do nothing
            break;
          }
        case Anki.Cozmo.DeviceDataType.BuildVersion: {
            _AppVersionLabel.FormattingArgs = new object[] { shortDataValue };
            break;
          }
        default: {
            DAS.Debug("SettingsVersionsPanel.HandleDeviceDataMessage.UnhandledDataType", currentPair.dataType.ToString());
            break;
          }
        }
      }
    }

    private void HandleOpenEraseCozmoViewButtonTapped() {
      if (_EraseCozmoModalInstance == null) {
        System.Action<BaseModal> eraseCozmoModalCreated = (newModal) => {
          _EraseCozmoModalInstance = (LongPressConfirmationModal)newModal;
          _EraseCozmoModalInstance.Initialize("erase_cozmo_dialog",
                                              LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoTitle,
                                              LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoWarningLabel,
                                              LocalizationKeys.kButtonCancel,
                                              LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoConfirmButton,
                                              HandleConfirmEraseCozmo,
                                              _SecondsToConfirmEraseCozmo);
          string buttonText = Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoConfirmButton);
          _EraseCozmoModalInstance.ShowInstructionsLabel(Localization.GetWithArgs(LocalizationKeys.kLabelPressAndHoldInstruction,
                                                                                   new object[] { buttonText }));
        };
        UIManager.OpenModal(AlertModalLoader.Instance.LongPressConfirmationModalPrefab, _SettingsModalPriorityData, eraseCozmoModalCreated);
      }
    }

    private void HandleConfirmEraseCozmo() {
      if (_EraseCozmoModalInstance != null) {
        RobotEngineManager.Instance.SetEnableAllReactionTriggers(false);
        _EraseCozmoModalInstance.ShowInProgressLabel(Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoInProgressLabel));
        _EraseCozmoModalInstance.EnableButtons(false);

        RobotEngineManager.Instance.AddCallback<RestoreRobotStatus>(HandleEraseRobotStatus);

        RobotEngineManager.Instance.CurrentRobot.WipeRobotGameData();
      }
    }

    private void HandleEraseRobotStatus(RestoreRobotStatus robotStatusMsg) {
      if (robotStatusMsg.isWipe) {
        RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleEraseRobotStatus);
        RobotEngineManager.Instance.SetEnableAllReactionTriggers(true);

        if (robotStatusMsg.success) {

          // Write the onboarding tag to this robot after erasing so robot stays in sync with app in terms of onboarding being completed
          Anki.Cozmo.OnboardingData data = new Anki.Cozmo.OnboardingData();
          data.hasCompletedOnboarding = true;
          byte[] byteArr = new byte[data.Size];
          System.IO.MemoryStream ms = new System.IO.MemoryStream(byteArr);
          data.Pack(ms);
          RobotEngineManager.Instance.CurrentRobot.NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_OnboardingData, byteArr);

          _EraseCozmoModalInstance.CloseDialog();
          PauseManager.Instance.StartPlayerInducedSleep(false);
        }
        else {
          _EraseCozmoModalInstance.ShowInstructionsLabel(Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoErrorLabel));
          _EraseCozmoModalInstance.EnableButtons(true);
        }
      }
    }

    private void HandleOpenSupportViewButtonTapped() {
      if (_SupportInfoModalInstance == null) {
        UIManager.OpenModal(_SupportInfoModalPrefab, _SettingsModalPriorityData, (newModal) => {
          _SupportInfoModalInstance = (SettingsSupportInfoModal)newModal;
          _SupportInfoModalInstance.OnOpenRestoreCozmoViewButtonTapped += HandleOpenRestoreCozmoViewButtonTapped;
          _SupportInfoModalInstance.HideRestoreButton(_RestoreButtonIsActive);
        });
      }
    }

    private void HandleOpenRestoreCozmoViewButtonTapped() {
      if (_RestoreCozmoModalInstance == null) {
        if (_SupportInfoModalInstance != null) {
          _SupportInfoModalInstance.CloseDialog();
        }

        System.Action<BaseModal> restoreCozmoModalCreated = (newModal) => {
          _RestoreCozmoModalInstance = (LongPressConfirmationModal)newModal;
          _RestoreCozmoModalInstance.Initialize("restore_cozmo_dialog",
                                                LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoTitle,
                                                LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoWarningLabel,
                                                LocalizationKeys.kButtonCancel,
                                                LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoConfirmButton,
                                                HandleRestoreCozmoConfirmed,
                                                _SecondsToConfirmRestoreCozmo);
          string buttonText = Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoConfirmButton);
          _RestoreCozmoModalInstance.ShowInstructionsLabel(Localization.GetWithArgs(LocalizationKeys.kLabelPressAndHoldInstruction,
                                                                                   new object[] { buttonText }));
        };

        var restoreCozmoModalPriorityData = ModalPriorityData.CreateSlightlyHigherData(_SettingsModalPriorityData);

        UIManager.OpenModal(AlertModalLoader.Instance.LongPressConfirmationModalPrefab, restoreCozmoModalPriorityData, restoreCozmoModalCreated);
      }
    }

    private void HandleRestoreCozmoConfirmed() {
      if (_RestoreCozmoModalInstance != null) {
        _RestoreCozmoModalInstance.ShowInProgressLabel(Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoInProgressLabel));
        _RestoreCozmoModalInstance.EnableButtons(false);

        RobotEngineManager.Instance.AddCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);

        // 0 denotes that engine will automatically choose a backup file to restore from
        RobotEngineManager.Instance.CurrentRobot.RestoreRobotFromBackup(0);
      }
    }

    private void HandleRestoreRobotStatus(RestoreRobotStatus robotStatusMsg) {
      if (!robotStatusMsg.isWipe) {
        RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);

        if (robotStatusMsg.success) {
          _RestoreCozmoModalInstance.CloseDialog();
          PauseManager.Instance.StartPlayerInducedSleep(false);
        }
        else {
          _RestoreCozmoModalInstance.ShowInstructionsLabel(Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoErrorLabel));
          _RestoreCozmoModalInstance.EnableButtons(true);
        }
      }
    }

    private void HandleRestoreRobotOptions(RestoreRobotOptions msg) {
      // If there is only one backup file then user will not be able to restore
      _RestoreButtonIsActive = (msg.robotsWithBackupData.Length > 1);
    }
  }
}
