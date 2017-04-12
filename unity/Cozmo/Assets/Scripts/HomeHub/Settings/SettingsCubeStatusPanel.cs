using Anki.Cozmo;
using Cozmo.UI;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsCubeStatusPanel : MonoBehaviour {

    [SerializeField]
    private BaseModal _CubeHelpModalPrefab;

    [SerializeField]
    private CozmoButton _RefreshBlockPoolButton;

    [SerializeField]
    private CozmoButton _ShowCubeHelpButton;

    private BaseModal _SettingsCubeHelpDialogInstance;

    [SerializeField]
    private HorizontalLayoutGroup _LightCubeButtonLayoutGroup;

    [SerializeField]
    private SettingsLightCubeButton _SettingsLightCubeButtonPrefab;

    private AlertModal _ConfirmBlockPoolRefreshView;

    private const string kDasEventViewController = "settings_cube_status_panel";

    private ModalPriorityData _SettingsModalPriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                                                 LowPriorityModalAction.CancelSelf,
                                                                                 HighPriorityModalAction.Stack);

    private void Awake() {
      _RefreshBlockPoolButton.Initialize(HandleRefreshBlockPoolTapped, "refresh_blockpool_button", kDasEventViewController);
      _ShowCubeHelpButton.Initialize(HandleOpenCubeHelpViewTapped, "show_cube_help_dialog_button", kDasEventViewController);
    }

    // Use this for initialization
    void Start() {
      // Create buttons in layout group, one for each ObjectType
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE1);
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE2);
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE3);
    }

    void OnDestroy() {
      if (_SettingsCubeHelpDialogInstance != null) {
        _SettingsCubeHelpDialogInstance.CloseDialogImmediately();
      }

      if (_ConfirmBlockPoolRefreshView != null) {
        _ConfirmBlockPoolRefreshView.CloseDialogImmediately();
      }
    }

    private SettingsLightCubeButton CreateSettingsLightCubeButton(ObjectType objectType) {
      SettingsLightCubeButton settingsLightCubeButton = UIManager.CreateUIElement(_SettingsLightCubeButtonPrefab,
                                                                                  _LightCubeButtonLayoutGroup.transform)
                                                                 .GetComponent<SettingsLightCubeButton>();
      string dasEventViewControllerName = kDasEventViewController;
      settingsLightCubeButton.Initialize(objectType, dasEventViewControllerName);
      return settingsLightCubeButton;
    }

    private void HandleRefreshBlockPoolTapped() {
      var refreshBlockPoolAlert = new AlertModalData("refresh_block_pool_alert",
                                                     LocalizationKeys.kSettingsCubeStatusPanelButtonCubeRefresh,
                                                     LocalizationKeys.kRefreshCubesPromptDescription,
                                                     new AlertModalButtonData("refresh_button", LocalizationKeys.kButtonRefresh, HandleRefreshBlockPool),
                                                     new AlertModalButtonData("cancel_button", LocalizationKeys.kButtonCancel));

      System.Action<AlertModal> refreshBlockPoolCreated = (alertModal) => {
        _ConfirmBlockPoolRefreshView = alertModal;
      };

      UIManager.OpenAlert(refreshBlockPoolAlert, _SettingsModalPriorityData, refreshBlockPoolCreated);
    }

    private void HandleRefreshBlockPool() {
      RobotEngineManager.Instance.BlockPoolTracker.ResetBlockPool();
    }

    private void HandleOpenCubeHelpViewTapped() {
      if (_SettingsCubeHelpDialogInstance == null) {
        UIManager.OpenModal(_CubeHelpModalPrefab, _SettingsModalPriorityData, (newView) => {
          _SettingsCubeHelpDialogInstance = newView;
        });
      }
    }
  }
}
