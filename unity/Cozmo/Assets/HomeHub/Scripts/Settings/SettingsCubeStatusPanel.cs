using Anki.Cozmo;
using Cozmo.UI;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SettingsCubeStatusPanel : MonoBehaviour {

    // TODO - Add Cube help dialog
    // [SerializeField]
    // private CozmoButton _ShowCubeHelpButton;

    // TODO - Add Cube help dialog
    // [SerializeField]
    // private SettingsCubeHelpDialog _SettingsCubeHelpDialogPrefab;
    // private SettingsCubeHelpDialog _SettingsCubeHelpDialogInstance;

    [SerializeField]
    private HorizontalLayoutGroup _LightCubeButtonLayoutGroup;

    [SerializeField]
    private SettingsLightCubeButton _SettingsLightCubeButtonPrefab;

    private List<ObjectType> _ButtonsRequestDisableBlockPool;

    // Use this for initialization
    void Start() {
      // TODO: Hook up ShowCubeHelpButton

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
      RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(true);
      RobotEngineManager.Instance.BlockPoolTracker.SendAvailableObjects(false,
                                                                        (byte)RobotEngineManager.Instance.CurrentRobotID);
      RobotEngineManager.Instance.CurrentRobot.EnableCubeLightsStateTransitionMessages(false);
    }

    private SettingsLightCubeButton CreateSettingsLightCubeButton(ObjectType objectType) {
      SettingsLightCubeButton settingsLightCubeButton = UIManager.CreateUIElement(_SettingsLightCubeButtonPrefab,
                                                                                  _LightCubeButtonLayoutGroup.transform)
                                                                 .GetComponent<SettingsLightCubeButton>();
      string dasEventViewControllerName = "settings_cube_status_panel";
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
  }
}
