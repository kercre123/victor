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
    private string _AcknowledgementsTextFileName;

    [SerializeField]
    private CozmoButton _PrivacyPolicyLinkButton;

    private ScrollingTextView _PrivacyPolicyDialogInstance;

    [SerializeField]
    private string _PrivacyPolicyTextFileName;

    [SerializeField]
    private CozmoButton _TermsOfUseButton;

    private ScrollingTextView _TermsOfUseDialogInstance;

    [SerializeField]
    private string _TermsOfUseTextFileName;

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

    private bool _RestoreButtonIsActive = true;

    private void Start() {
      RobotEngineManager.Instance.AddCallback<DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.AddCallback<RestoreRobotOptions>(HandleRestoreRobotOptions);
      RobotEngineManager.Instance.SendRequestDeviceData();

      // Fill out Cozmo version args
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      _CozmoVersionLabel.FormattingArgs = new object[] { ShortenData(robot.FirmwareVersion.ToString()) };

      // Fill out Serial number args
      _SerialNumberLabel.FormattingArgs = new object[] { robot.SerialNumber.ToString("X8") };

      string dasEventViewName = "settings_version_panel";
      _AcknowledgementsLinkButton.Initialize(HandleAcknowledgementsLinkButtonTapped, "acknowledgements_link", dasEventViewName);
      _PrivacyPolicyLinkButton.Initialize(HandlePrivacyPolicyLinkButtonTapped, "privacyPolicy_link", dasEventViewName);
      _TermsOfUseButton.Initialize(HandleTermsOfUseLinkButtonTapped, "termsOfUse_link", dasEventViewName);

      _SupportButton.Initialize(HandleOpenSupportViewButtonTapped, "support_button", dasEventViewName);

      _EraseCozmoButton.Initialize(HandleOpenEraseCozmoViewButtonTapped, "open_erase_cozmo_view_button", dasEventViewName);

      robot.RequestRobotRestoreData();
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleEraseRobotStatus);
      if (_AcknowledgementsDialogInstance != null) {
        _AcknowledgementsDialogInstance.CloseViewImmediately();
      }
      if (_PrivacyPolicyDialogInstance != null) {
        _PrivacyPolicyDialogInstance.CloseViewImmediately();
      }
      if (_TermsOfUseDialogInstance != null) {
        _TermsOfUseDialogInstance.CloseViewImmediately();
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
        _AcknowledgementsDialogInstance.Initialize(Localization.Get(LocalizationKeys.kSettingsVersionPanelAcknowledgementsModalTitle),
                                                   Localization.ReadLocalizedTextFromFile(_AcknowledgementsTextFileName));
      }
    }

    private void HandlePrivacyPolicyLinkButtonTapped() {
      if (_PrivacyPolicyDialogInstance == null) {
        _PrivacyPolicyDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.ScrollingTextViewPrefab,
                                                           (ScrollingTextView view) => { view.DASEventViewName = "privacyPolicy_view"; });
        _PrivacyPolicyDialogInstance.Initialize(Localization.Get(LocalizationKeys.kPrivacyPolicyTitle),
                                                Localization.ReadLocalizedTextFromFile(_PrivacyPolicyTextFileName));
      }
    }

    private void HandleTermsOfUseLinkButtonTapped() {
      if (_TermsOfUseDialogInstance == null) {
        _TermsOfUseDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.ScrollingTextViewPrefab,
                                                       (ScrollingTextView view) => { view.DASEventViewName = "termsOfUse_view"; });
        _TermsOfUseDialogInstance.Initialize(Localization.Get(LocalizationKeys.kLabelTermsOfUse),
                                             Localization.ReadLocalizedTextFromFile(_TermsOfUseTextFileName));
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
        RobotEngineManager.Instance.SetEnableReactionaryBehaviors(false);
        _EraseCozmoDialogInstance.ShowInProgressLabel(Localization.Get(LocalizationKeys.kSettingsVersionPanelEraseCozmoModalEraseCozmoInProgressLabel));
        _EraseCozmoDialogInstance.EnableButtons(false);

        RobotEngineManager.Instance.AddCallback<RestoreRobotStatus>(HandleEraseRobotStatus);

        RobotEngineManager.Instance.CurrentRobot.WipeRobotGameData();
      }
    }

    private void HandleEraseRobotStatus(RestoreRobotStatus robotStatusMsg) {
      if (robotStatusMsg.isWipe) {
        RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleEraseRobotStatus);
        RobotEngineManager.Instance.SetEnableReactionaryBehaviors(true);

        if (robotStatusMsg.success) {

          // Write the onboarding tag to this robot after erasing so robot stays in sync with app in terms of onboarding being completed
          Anki.Cozmo.OnboardingData data = new Anki.Cozmo.OnboardingData();
          data.hasCompletedOnboarding = true;
          byte[] byteArr = new byte[data.Size];
          System.IO.MemoryStream ms = new System.IO.MemoryStream(byteArr);
          data.Pack(ms);
          RobotEngineManager.Instance.CurrentRobot.NVStorageWrite(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_OnboardingData, byteArr);

          _EraseCozmoDialogInstance.CloseView();
          PauseManager.Instance.StartPlayerInducedSleep(false);
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
        _SupportInfoViewInstance.HideRestoreButton(_RestoreButtonIsActive);
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
      if (!robotStatusMsg.isWipe) {
        RobotEngineManager.Instance.RemoveCallback<RestoreRobotStatus>(HandleRestoreRobotStatus);

        if (robotStatusMsg.success) {
          _RestoreCozmoDialogInstance.CloseView();
          PauseManager.Instance.StartPlayerInducedSleep(false);
        }
        else {
          _RestoreCozmoDialogInstance.ShowInstructionsLabel(Localization.Get(LocalizationKeys.kSettingsSupportViewRestoreCozmoModalRestoreCozmoErrorLabel));
          _RestoreCozmoDialogInstance.EnableButtons(true);
        }
      }
    }

    private void HandleRestoreRobotOptions(RestoreRobotOptions msg) {
      // If there is only one backup file then user will not be able to restore
      _RestoreButtonIsActive = (msg.robotsWithBackupData.Length > 1);
    }
  }
}