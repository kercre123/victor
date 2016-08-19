using Cozmo.UI;
using DataPersistence;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsSDKPanel : MonoBehaviour {
    [SerializeField]
    private CozmoButton _EnableSDKButton;

    [SerializeField]
    private CozmoButton _ShowSDKButton;

    [SerializeField]
    private CozmoButton _AnkiSDKLinkButton;

    // Use this for initialization
    private void Start() {
      bool isSDKEnabled = DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled;
      _EnableSDKButton.gameObject.SetActive(!isSDKEnabled);
      _ShowSDKButton.gameObject.SetActive(isSDKEnabled);

      _EnableSDKButton.Initialize(HandleEnableSDKButtonTapped, "enable_sdk_button", "settings_sdk_panel");
      _ShowSDKButton.Initialize(HandleShowSDKUIButtonTapped, "show_sdk_ui_button", "settings_sdk_panel");
      _AnkiSDKLinkButton.Initialize(HandleSDKLinkButtonTapped, "sdk_link_button", "settings_sdk_panel");
    }

    private void HandleEnableSDKButtonTapped() {
      DAS.Info("SettingsSDKPanel.HandleEnableSDKButtonTapped", "Enable SDK Button tapped!");
      _EnableSDKButton.gameObject.SetActive(false);
      _ShowSDKButton.gameObject.SetActive(true);

      DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled = true;
      DataPersistenceManager.Instance.Save();

      // TODO: Show SDK UI on first enabling SDK
    }

    private void HandleShowSDKUIButtonTapped() {
      DAS.Info("SettingsSDKPanel.HandleShowSDKUIButtonTapped", "Show SDK UI Button tapped!");
      // TODO: Open SDK UI here
    }

    private void HandleSDKLinkButtonTapped() {
      Application.OpenURL(DefaultSettingsValuesConfig.Instance.SdkUrl);
    }
  }
}