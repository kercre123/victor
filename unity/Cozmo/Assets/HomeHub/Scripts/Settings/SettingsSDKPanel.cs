using Cozmo.UI;
using DataPersistence;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsSDKPanel : MonoBehaviour {
    [SerializeField]
    private CozmoButton _EnableSDKButton;

    [SerializeField]
    private SDKModal _SDKModalPrefab;

    private AlertModal _ActivateSDKModal = null;

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
          var enableSDKButtonData = new AlertModalButtonData("confirm_button", LocalizationKeys.kButtonYes, EnableSDK);
          var cancelSDKButtonData = new AlertModalButtonData("cancel_button", LocalizationKeys.kButtonNo, HandleCloseSDKPopup);

          var confirmEnableSDKAlert = new AlertModalData("confirm_enable_sdk_alert",
                                                         LocalizationKeys.kSettingsSdkPanelActivateSDKalertText,
                                                         primaryButtonData: enableSDKButtonData,
                                                         secondaryButtonData: cancelSDKButtonData);

          var confirmEnableSDKPriority = new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                               LowPriorityModalAction.CancelSelf,
                                                               HighPriorityModalAction.Stack);

          System.Action<AlertModal> confirmEnableSDKCreated = (alertView) => {
            _ActivateSDKModal = alertView;
          };

          UIManager.OpenAlert(confirmEnableSDKAlert, confirmEnableSDKPriority, confirmEnableSDKCreated,
                              overrideCloseOnTouchOutside: true);
        }
      }
    }

    private void EnableSDK() {
      DataPersistenceManager.Instance.IsSDKEnabled = true;
      DataPersistenceManager.Instance.Data.DeviceSettings.SDKActivated = true;
      DataPersistenceManager.Instance.Save();

      var sdkModalPriorityData = new UI.ModalPriorityData(ModalPriorityLayer.VeryHigh, 0,
                                                          LowPriorityModalAction.Queue,
                                                          HighPriorityModalAction.ForceCloseOthersAndOpen);
      UIManager.OpenModal(_SDKModalPrefab, sdkModalPriorityData, null);
    }

    private void HandleCloseSDKPopup() {
      if (_ActivateSDKModal != null) {
        _ActivateSDKModal.CloseDialog();
      }
    }

  }
}