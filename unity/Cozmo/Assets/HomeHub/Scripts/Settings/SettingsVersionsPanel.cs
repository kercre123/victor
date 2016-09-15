using Anki.Cozmo.ExternalInterface;
using Anki.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsVersionsPanel : MonoBehaviour {

    [SerializeField]
    private AnkiTextLabel _AppVersionLabel;

    [SerializeField]
    private AnkiTextLabel _CozmoVersionLabel;

    [SerializeField]
    private AnkiTextLabel _SerialNumberLabel;

    [SerializeField]
    private AnkiTextLabel _DeviceIDLabel;

    [SerializeField]
    private AnkiTextLabel _AppRunLabel;

    [SerializeField]
    private CozmoButton _AcknowledgementsLinkButton;

    private ScrollingTextView _AcknowledgementsDialogInstance;

    [SerializeField]
    private CozmoButton _SupportButton;

    [SerializeField]
    private SettingsSupportInfoView _SupportInfoViewPrefab;

    private SettingsSupportInfoView _SupportInfoViewInstance;

    [SerializeField]
    private CozmoButton _EraseCozmoButton;

    [SerializeField]
    private float _SecondsToConfirmEraseCozmo = 5f;

    private LongConfirmationView _EraseCozmoDialogInstance;

    [SerializeField]
    private float _SecondsToConfirmRestoreCozmo = 5f;

    private LongConfirmationView _RestoreCozmoDialogInstance;

    private void Start() {
      RobotEngineManager.Instance.AddCallback<DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.SendRequestDeviceData();

      // Fill out Cozmo version args
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      _CozmoVersionLabel.FormattingArgs = new object[] { ShortenData(robot.FirmwareVersion.ToString()) };

      // Fill out Serial number args
      _SerialNumberLabel.FormattingArgs = new object[] { ShortenData(robot.SerialNumber.ToString("X6")) };

      string dasEventViewName = "settings_version_panel";
      _AcknowledgementsLinkButton.Initialize(HandleAcknowledgementsLinkButtonTapped, "acknowledgements_link", dasEventViewName);

      _SupportButton.Initialize(HandleOpenSupportViewButtonTapped, "support_button", dasEventViewName);

      _EraseCozmoButton.Initialize(HandleOpenEraseCozmoViewButtonTapped, "open_erase_cozmo_view_button", dasEventViewName);
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleEraseRobotStatus);
      if (_AcknowledgementsDialogInstance != null) {
        _AcknowledgementsDialogInstance.CloseViewImmediately();
      }
      if (_SupportInfoViewInstance != null) {
        _SupportInfoViewInstance.CloseViewImmediately();
        _SupportInfoViewInstance.OnOpenRestoreCozmoViewButtonTapped -= HandleOpenRestoreCozmoViewButtonTapped;
      }
      if (_EraseCozmoDialogInstance != null) {
        _EraseCozmoDialogInstance.CloseViewImmediately();
      }

      RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);
      if (_RestoreCozmoDialogInstance != null) {
        _RestoreCozmoDialogInstance.CloseViewImmediately();
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

    private void HandleAcknowledgementsLinkButtonTapped() {
      if (_AcknowledgementsDialogInstance == null) {
        _AcknowledgementsDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.ScrollingTextViewPrefab,
                                                             (ScrollingTextView view) => { view.DASEventViewName = "acknowledgements_view"; });
        _AcknowledgementsDialogInstance.Initialize(LocalizationKeys.kSettingsVersionPanelAcknowledgementsModalTitle,
                                                   LocalizationKeys.kSettingsVersionPanelAcknowledgementsModalDescription);
      }
    }

    private void HandleOpenEraseCozmoViewButtonTapped() {
      if (_EraseCozmoDialogInstance == null) {
        _EraseCozmoDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.LongConfirmationViewPrefab);
        _EraseCozmoDialogInstance.Initialize("erase_cozmo_dialog",
                                             LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoTitle,
                                             LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoWarningLabel,
                                             LocalizationKeys.kButtonCancel,
                                             LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoConfirmButton,
                                             HandleConfirmEraseCozmo,
                                             _SecondsToConfirmEraseCozmo);
        string buttonText = Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoConfirmButton);
        _EraseCozmoDialogInstance.ShowInstructionsLabel(Localization.GetWithArgs(LocalizationKeys.kLabelPressAndHoldInstruction,
                                                                                 new object[] { buttonText }));
      }
    }

    private void HandleConfirmEraseCozmo() {
      if (_EraseCozmoDialogInstance != null) {
        _EraseCozmoDialogInstance.ShowInProgressLabel(Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoInProgressLabel));
        _EraseCozmoDialogInstance.EnableButtons(false);

        RobotEngineManager.Instance.AddCallback<RestoreRobotStatus>(HandleEraseRobotStatus);

        RobotEngineManager.Instance.CurrentRobot.WipeRobotGameData();
      }
    }

    private void HandleEraseRobotStatus(RestoreRobotStatus robotStatusMsg) {
      if (robotStatusMsg.didWipe) {
        RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleEraseRobotStatus);

        if (robotStatusMsg.success) {
          _EraseCozmoDialogInstance.CloseView();
          PauseManager.Instance.StartPlayerInducedSleep();
        }
        else {
          _EraseCozmoDialogInstance.ShowInstructionsLabel(Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoErrorLabel));
          _EraseCozmoDialogInstance.EnableButtons(true);
        }
      }
    }

    private void HandleOpenSupportViewButtonTapped() {
      if (_SupportInfoViewInstance == null) {
        _SupportInfoViewInstance = UIManager.OpenView(_SupportInfoViewPrefab);
        _SupportInfoViewInstance.OnOpenRestoreCozmoViewButtonTapped += HandleOpenRestoreCozmoViewButtonTapped;
      }
    }

    private void HandleOpenRestoreCozmoViewButtonTapped() {
      if (_RestoreCozmoDialogInstance == null) {
        if (_SupportInfoViewInstance != null) {
          _SupportInfoViewInstance.CloseView();
        }
        _RestoreCozmoDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.LongConfirmationViewPrefab);
        _RestoreCozmoDialogInstance.Initialize("restore_cozmo_dialog",
                                               LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoTitle,
                                               LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoWarningLabel,
                                               LocalizationKeys.kButtonCancel,
                                               LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoConfirmButton,
                                               HandleRestoreCozmoConfirmed,
                                               _SecondsToConfirmRestoreCozmo);
        string buttonText = Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoConfirmButton);
        _RestoreCozmoDialogInstance.ShowInstructionsLabel(Localization.GetWithArgs(LocalizationKeys.kLabelPressAndHoldInstruction,
                                                                                 new object[] { buttonText }));
      }
    }

    private void HandleRestoreCozmoConfirmed() {
      if (_RestoreCozmoDialogInstance != null) {
        _RestoreCozmoDialogInstance.ShowInProgressLabel(Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoInProgressLabel));
        _RestoreCozmoDialogInstance.EnableButtons(false);

        RobotEngineManager.Instance.AddCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);

        // 0 denotes that engine will automatically choose a backup file to restore from
        RobotEngineManager.Instance.CurrentRobot.RestoreRobotFromBackup(0);
      }
    }

    private void HandleRestoreRobotStatus(RestoreRobotStatus robotStatusMsg) {
      if (!robotStatusMsg.didWipe) {
        RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);

        if (robotStatusMsg.success) {
          _RestoreCozmoDialogInstance.CloseView();
          PauseManager.Instance.StartPlayerInducedSleep();
        }
        else {
          _RestoreCozmoDialogInstance.ShowInstructionsLabel(Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoErrorLabel));
          _RestoreCozmoDialogInstance.EnableButtons(true);
        }
      }
    }
  }
}