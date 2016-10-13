using Cozmo.UI;
using DataPersistence;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsSDKPanel : MonoBehaviour {
    [SerializeField]
    private CozmoButton _EnableSDKButton;

    [SerializeField]
    private SDKView _SDKViewPrefab;

    private AlertView _ActivateSDKModal = null;

    private void Awake() {
      _EnableSDKButton.Initialize(HandleEnableSDKButtonTapped, "enable_sdk_button", "settings_sdk_panel");
    }

    // Use this for initialization
    private void Start() {
      // Immediately open the SDK UI upon opening this view if the SDK is enabled
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        EnableSDK();
      }
    }

    private void HandleEnableSDKButtonTapped() {
      DAS.Info("SettingsSDKPanel.HandleEnableSDKButtonTapped", "Enable SDK Button tapped!");
      if (_ActivateSDKModal == null) {
        // If the SDK has already been activated, skip the confirmation modal
        if (DataPersistenceManager.Instance.Data.DeviceSettings.SDKActivated) {
          EnableSDK();
        }
        else {
          // If this is the first time enabling the SDK, display a confirmation modal
          // with EULA and only EnableSDK on confirmation.
          // Create alert view with Icon
          AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_NoText, overrideCloseOnTouchOutside: true);
          // Hook up callbacks
          alertView.SetCloseButtonEnabled(false);
          alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, EnableSDK);
          alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleCloseSDKPopup);
          alertView.TitleLocKey = LocalizationKeys.kSettingsSdkPanelActivateSDKalertText;
          _ActivateSDKModal = alertView;
        }
      }

    }

    private void EnableSDK() {

      DataPersistenceManager.Instance.IsSDKEnabled = true;
      DataPersistenceManager.Instance.Data.DeviceSettings.SDKActivated = true;
      DataPersistenceManager.Instance.Save();
      UIManager.OpenView(_SDKViewPrefab);

    }

    private void HandleCloseSDKPopup() {
      if (_ActivateSDKModal != null) {
        _ActivateSDKModal.CloseView();
      }
    }

  }
}