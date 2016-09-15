using Anki.Cozmo;
using Cozmo.UI;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsCubeStatusPanel : MonoBehaviour {

    [SerializeField]
    private CozmoButton _ShowCubeHelpButton;

    [SerializeField]
    private BaseView _SettingsCubeHelpDialogPrefab;
    private BaseView _SettingsCubeHelpDialogInstance;

    [SerializeField]
    private HorizontalLayoutGroup _LightCubeButtonLayoutGroup;

    [SerializeField]
    private SettingsLightCubeButton _SettingsLightCubeButtonPrefab;

    private List<ObjectType> _ButtonsRequestDisableBlockPool;

    private const string kDasEventViewController = "settings_cube_status_panel";

    // Use this for initialization
    void Start() {
      _ShowCubeHelpButton.Initialize(HandleOpenCubeHelpViewTapped, "show_cube_help_dialog_button", kDasEventViewController);

      _ButtonsRequestDisableBlockPool = new List<ObjectType>();

      // Create buttons in layout group, one for each ObjectType
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE1);
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE2);
      CreateSettingsLightCubeButton(ObjectType.Block_LIGHTCUBE3);

      RobotEngineManager.Instance.BlockPoolTracker.SendAvailableObjects(true,
                                                                        (byte)RobotEngineManager.Instance.CurrentRobotID);
      RobotEngineManager.Instance.CurrentRobot.EnableCubeLightsStateTransitionMessages(true);
    }

    void OnDestroy() {
      if (_SettingsCubeHelpDialogInstance != null) {
        _SettingsCubeHelpDialogInstance.CloseViewImmediately();
      }
      RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(true);
      RobotEngineManager.Instance.BlockPoolTracker.SendAvailableObjects(false,
                                                                        (byte)RobotEngineManager.Instance.CurrentRobotID);
      RobotEngineManager.Instance.CurrentRobot.EnableCubeLightsStateTransitionMessages(false);
    }

    private SettingsLightCubeButton CreateSettingsLightCubeButton(ObjectType objectType) {
      SettingsLightCubeButton settingsLightCubeButton = UIManager.CreateUIElement(_SettingsLightCubeButtonPrefab,
                                                                                  _LightCubeButtonLayoutGroup.transform)
                                                                 .GetComponent<SettingsLightCubeButton>();
      string dasEventViewControllerName = kDasEventViewController;
      settingsLightCubeButton.Initialize(this, objectType, dasEventViewControllerName);
      return settingsLightCubeButton;
    }

    public void DisableAutomaticBlockPool(ObjectType buttonRequesting) {
      if (!_ButtonsRequestDisableBlockPool.Contains(buttonRequesting)) {
        _ButtonsRequestDisableBlockPool.Add(buttonRequesting);
        if (_ButtonsRequestDisableBlockPool.Count == 1) {
          RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(false);
        }
      }
    }

    public void EnableAutomaticBlockPool(ObjectType buttonRequesting) {
      if (_ButtonsRequestDisableBlockPool.Contains(buttonRequesting)) {
        _ButtonsRequestDisableBlockPool.Remove(buttonRequesting);
        if (_ButtonsRequestDisableBlockPool.Count == 0) {
          RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(true);
        }
      }
    }

    private void HandleOpenCubeHelpViewTapped() {
      if (_SettingsCubeHelpDialogInstance == null) {
        _SettingsCubeHelpDialogInstance = UIManager.OpenView(_SettingsCubeHelpDialogPrefab);
      }
    }
  }
}
