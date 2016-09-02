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

    private void Start() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
      RobotEngineManager.Instance.SendRequestDeviceData();

      // Fill out Cozmo version args
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      _CozmoVersionLabel.FormattingArgs = new object[] { robot.FirmwareVersion };

      // Fill out Serial number args
      _SerialNumberLabel.FormattingArgs = new object[] { robot.SerialNumber };

      _AcknowledgementsLinkButton.Initialize(HandleAcknowledgementsLinkButtonTapped, "acknowledgements_link", "settings_version_panel");
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
    }

    private void HandleDeviceDataMessage(Anki.Cozmo.ExternalInterface.DeviceDataMessage message) {
      for (int i = 0; i < message.dataList.Length; ++i) {
        Anki.Cozmo.DeviceDataPair currentPair = message.dataList[i];
        string shortDataValue = currentPair.dataValue.Substring(0, DefaultSettingsValuesConfig.Instance.CharactersOfAppInfoToShow);
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
      // TODO INGO: Show some kind of dialog here
      DAS.Info("SettingsVersionsPanel.HandleAcknowledgementsLinkButtonTapped", "Acknowledgements button tapped!");
    }
  }
}