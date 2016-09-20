using Anki.Cozmo;
using Cozmo.UI;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsCubeStatusPanel : MonoBehaviour {

    [SerializeField]
    private CozmoButton _RefreshBlockPoolButton;

    [SerializeField]
    private CozmoButton _ShowCubeHelpButton;

    private BaseView _SettingsCubeHelpDialogInstance;

    [SerializeField]
    private HorizontalLayoutGroup _LightCubeButtonLayoutGroup;

    [SerializeField]
    private SettingsLightCubeButton _SettingsLightCubeButtonPrefab;

    private AlertView _ConfirmBlockPoolRefreshView;

    private const string kDasEventViewController = "settings_cube_status_panel";

    // Use this for initialization
    void Start() {
      _RefreshBlockPoolButton.Initialize(HandleRefreshBlockPoolTapped, "refresh_blockpool_button", kDasEventViewController);
      _ShowCubeHelpButton.Initialize(HandleOpenCubeHelpViewTapped, "show_cube_help_dialog_button", kDasEventViewController);

      // Create buttons in layout group, one for each ObjectType
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE1);
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE2);
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE3);
    }

    void OnDestroy() {
      if (_SettingsCubeHelpDialogInstance != null) {
        _SettingsCubeHelpDialogInstance.CloseViewImmediately();
      }

      if (_ConfirmBlockPoolRefreshView != null) {
        _ConfirmBlockPoolRefreshView.CloseViewImmediately();
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
      AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab);
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonRefresh, HandleRefreshBlockPool);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel, null);
      alertView.TitleLocKey = LocalizationKeys.kSettingsCubeStatusPanelButtonCubeRefresh;
      alertView.DescriptionLocKey = LocalizationKeys.kRefreshCubesPromptDescription;

      _ConfirmBlockPoolRefreshView = alertView;
    }

    private void HandleRefreshBlockPool() {
      RobotEngineManager.Instance.BlockPoolTracker.ResetBlockPool();
    }

    private void HandleOpenCubeHelpViewTapped() {
      if (_SettingsCubeHelpDialogInstance == null) {
        _SettingsCubeHelpDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.CubeHelpViewPrefab);
      }
    }
  }
}
