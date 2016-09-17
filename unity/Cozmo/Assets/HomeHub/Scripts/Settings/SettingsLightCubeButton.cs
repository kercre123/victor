using Anki.Cozmo;
using Cozmo.BlockPool;
using Cozmo.UI;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsLightCubeButton : MonoBehaviour {

    [SerializeField, Range(0f, 1f)]
    private float _AlphaWhenNoBlockFound = 0.5f;

    [SerializeField]
    private CozmoButton _LightUpCubeButton;

    [SerializeField]
    private LightCubeSprite _LightCubeSprite;

    [SerializeField]
    private GameObject _CubeStatusMissingObject;

    [SerializeField]
    private GameObject _CubeStatusFoundObject;

    private ObjectType _LightCubeType;

    private LightCube _LightCube;
    private IRobot _CurrentRobot;

    public void Initialize(ObjectType objectType, string dasEventViewController) {
      _LightCubeType = objectType;

      // Set icon on sprite based on objectType and initialize the button with default values
      _LightCubeSprite.SetIcon(_LightCubeType);
      FollowLightCube(null);

      // Get light cube that matches object type from robot
      _CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (_CurrentRobot != null) {
        foreach (var cube in _CurrentRobot.LightCubes) {
          if (cube.Value.ObjectType == _LightCubeType) {
            FollowLightCube(cube.Value);
            break;
          }
        }

        // Register for cube added/removed events
        _CurrentRobot.OnLightCubeAdded += HandleLightCubeAdded;
        _CurrentRobot.OnLightCubeRemoved += HandleLightCubeRemoved;
      }

      _LightUpCubeButton.Initialize(HandleFlashCubeButtonPressed, "flash_cube_button_" + objectType, dasEventViewController);
    }

    private void OnDestroy() {
      if (_CurrentRobot != null) {
        _CurrentRobot.OnLightCubeAdded -= HandleLightCubeAdded;
        _CurrentRobot.OnLightCubeRemoved -= HandleLightCubeRemoved;
      }
    }

    private void HandleLightCubeAdded(LightCube cube) {
      if ((_LightCube == null) && (cube.ObjectType == _LightCubeType)) {
        FollowLightCube(cube);
      }
    }

    private void HandleLightCubeRemoved(LightCube cube) {
      if ((_LightCube != null) && (cube.FactoryID == _LightCube.FactoryID)) {
        FollowLightCube(null);
      }
    }

    private void FollowLightCube(LightCube cube) {
      _LightCube = cube;
      if (cube == null) {
        _LightCubeSprite.SetAlpha(_AlphaWhenNoBlockFound);
        _LightUpCubeButton.Interactable = false;
        _CubeStatusMissingObject.SetActive(true);
        _CubeStatusFoundObject.SetActive(false);
      }
      else {
        _LightCubeSprite.SetAlpha(1f);
        _LightUpCubeButton.Interactable = true;
        _CubeStatusMissingObject.SetActive(false);
        _CubeStatusFoundObject.SetActive(true);
      }
    }

    private void HandleFlashCubeButtonPressed() {
      if ((_CurrentRobot != null) && (_LightCube != null)) {
        _CurrentRobot.FlashCurrentLightsState((uint)_LightCube.ID);
      }
    }
  }
}