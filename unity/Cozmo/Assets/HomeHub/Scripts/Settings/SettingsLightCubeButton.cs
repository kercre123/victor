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
    private CozmoButton _SwapCubeButton;

    [SerializeField]
    private RectTransform _LightCubeSpriteContainer;

    private LightCubeSprite _LightCubeSprite;

    private ObjectType _LightCubeType;
    private List<uint> _AttemptedConnectionFactoryIds;

    private LightCube _CurrentLightCube;
    private IRobot _CurrentRobot;
    private SettingsCubeStatusPanel _ParentPanel;

    public void Initialize(SettingsCubeStatusPanel parentPanel, ObjectType objectType, string dasEventViewController) {
      _ParentPanel = parentPanel;
      _AttemptedConnectionFactoryIds = new List<uint>();

      _LightCubeType = objectType;
      _LightCubeSprite = UIManager.CreateUIElement(CubePalette.Instance.LightCubeSpritePrefab,
                                                   _LightCubeSpriteContainer)
                                  .GetComponent<LightCubeSprite>();

      DimSprite();

      // Set icon on sprite based on objectType
      _LightCubeSprite.SetIcon(_LightCubeType);

      // Get light cube that matches object type from robot
      _CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      LightCube lightCubeToFollow = null;
      foreach (var cube in _CurrentRobot.LightCubes) {
        if (cube.Value.ObjectType == _LightCubeType) {
          lightCubeToFollow = cube.Value;
          break;
        }
      }

      _LightUpCubeButton.Initialize(HandleFlashCubeButtonPressed, "flash_cube_button_" + objectType, dasEventViewController);

      // If found, listen to light cube for light updates and add factoryId to attempted list
      if (lightCubeToFollow != null) {
        FollowLightCube(lightCubeToFollow);
      }
      // If not found, listen for cube connection events from Robot
      else {
        WaitForNextLightCube();
      }

      // Show swap button if there is more than one of this kind of block
      _SwapCubeButton.Initialize(HandleSwapCubeButtonPressed, "swap_cube_button_" + objectType, dasEventViewController);
      UpdateSwapCubeButton();

      // Listen for changes in number of available lightcubes of BlockPoolTracker
      RobotEngineManager.Instance.BlockPoolTracker.OnNumberAvailableObjectsChanged += HandleNumberAvailableObjectTypeChanged;
    }

    private void OnDestroy() {
      _CurrentRobot.OnLightCubeAdded -= HandleLightCubeAdded;
      _CurrentRobot.OnLightCubeRemoved -= HandleLightCubeRemoved;
      _CurrentRobot.OnLightCubeRemoved -= HandleSwapLightCubeRemoved;
      _CurrentRobot.OnLightCubeAdded -= HandleSwapLightCubeAdded;
      RobotEngineManager.Instance.BlockPoolTracker.OnNumberAvailableObjectsChanged -= HandleNumberAvailableObjectTypeChanged;
    }

    private void HandleLightCubeAdded(LightCube cube) {
      if (_CurrentLightCube == null && cube.ObjectType == _LightCubeType) {
        FollowLightCube(cube);
      }
    }

    private void HandleLightCubeRemoved(LightCube cube) {
      if (_CurrentLightCube != null && cube.FactoryID == _CurrentLightCube.FactoryID) {
        WaitForNextLightCube();
      }
    }

    private void FollowLightCube(LightCube cube) {
      _CurrentLightCube = cube;
      _LightCubeSprite.SetAlpha(1f);
      _AttemptedConnectionFactoryIds.Add(_CurrentLightCube.FactoryID);

      // Listen to light updates from engine
      _LightCubeSprite.FollowLightCube(_CurrentLightCube, isFreeplay: true);

      // Enable flash cube button
      _LightUpCubeButton.Interactable = true;

      _SwapCubeButton.Interactable = true;

      // Listen for deletions to handle disconnect
      _CurrentRobot.OnLightCubeAdded -= HandleLightCubeAdded;
      _CurrentRobot.OnLightCubeRemoved += HandleLightCubeRemoved;
    }

    private void WaitForNextLightCube() {
      ResetCubeUI();
      _CurrentLightCube = null;

      _CurrentRobot.OnLightCubeRemoved -= HandleLightCubeRemoved;
      _CurrentRobot.OnLightCubeAdded += HandleLightCubeAdded;

      UpdateSwapCubeButton();
    }

    private void ResetCubeUI() {
      DimSprite();
      if (_CurrentLightCube != null) {
        _LightCubeSprite.StopFollowLightCube();
      }

      // Disable flash cube button
      _LightUpCubeButton.Interactable = false;
    }

    private void DimSprite() {
      _LightCubeSprite.SetAlpha(_AlphaWhenNoBlockFound);
      _LightCubeSprite.SetColor(Color.black);
    }

    private void HandleSwapCubeButtonPressed() {
      // Disable swap cube button
      _SwapCubeButton.Interactable = false;

      // Use BlockPoolTracker to select a different cube that's not the same as what's in _AttemptedConnectionFactoryIds
      BlockPoolTracker bpt = RobotEngineManager.Instance.BlockPoolTracker;
      List<uint> availableCubes = new List<uint>();
      bpt.ForEachObjectOfType(_LightCubeType, (BlockPoolData data) => {
        if (data.IsAvailable) {
          availableCubes.Add(data.FactoryID);
        }
      });

      if (availableCubes.Count >= 1) {
        // Stop listening to LightCube to update lights
        ResetCubeUI();

        _CurrentRobot.OnLightCubeAdded -= HandleLightCubeAdded;
        _CurrentRobot.OnLightCubeRemoved -= HandleLightCubeRemoved;

        _ParentPanel.DisableAutomaticBlockPool(_LightCubeType);
        if (_CurrentLightCube != null) {
          DisconnectFromCurrentCube();
        }
        else {
          FindAndConnectToNewCube(0);
        }
      }
      else {
        _SwapCubeButton.gameObject.SetActive(false);
      }
    }

    private void DisconnectFromCurrentCube() {
      // Use BlockPoolTracker to deselect
      BlockPoolTracker bpt = RobotEngineManager.Instance.BlockPoolTracker;
      uint oldFactoryId = _CurrentLightCube.FactoryID;
      bpt.SetObjectInPool(oldFactoryId, false);

      _CurrentRobot.OnLightCubeRemoved += HandleSwapLightCubeRemoved;
    }

    private void HandleSwapLightCubeRemoved(LightCube cube) {
      if (_CurrentLightCube != null && cube.FactoryID == _CurrentLightCube.FactoryID) {
        FindAndConnectToNewCube(_CurrentLightCube.FactoryID);
        _CurrentLightCube = null;
      }
    }

    private void FindAndConnectToNewCube(uint oldFactoryId) {
      BlockPoolTracker bpt = RobotEngineManager.Instance.BlockPoolTracker;
      List<uint> availableFactoryIdsOfType = new List<uint>();
      bpt.ForEachObjectOfType(_LightCubeType, (BlockPoolData data) => {
        if (data.IsAvailable) {
          availableFactoryIdsOfType.Add(data.FactoryID);
        }
      });
      uint cubeToSelect = SelectCubeToSwapTo(oldFactoryId, availableFactoryIdsOfType);
      bpt.SetObjectInPool(cubeToSelect, true);

      _CurrentRobot.OnLightCubeRemoved -= HandleSwapLightCubeRemoved;
      _CurrentRobot.OnLightCubeAdded += HandleSwapLightCubeAdded;
    }

    private void HandleSwapLightCubeAdded(LightCube cube) {
      if (_CurrentLightCube == null && cube.ObjectType == _LightCubeType) {
        _CurrentRobot.OnLightCubeAdded -= HandleSwapLightCubeAdded;
        _SwapCubeButton.Interactable = true;
        FollowLightCube(cube);
        _ParentPanel.EnableAutomaticBlockPool(_LightCubeType);
      }
    }

    private uint SelectCubeToSwapTo(uint oldFactoryId, List<uint> availableCubes) {
      // If _AttemptedConnectionFactoryIds.Count is the same as # of known available light cubes, reset the list
      uint selectedFactoryId = oldFactoryId;
      if (_AttemptedConnectionFactoryIds.Count >= availableCubes.Count) {
        _AttemptedConnectionFactoryIds.Clear();
      }

      BlockPoolTracker bpt = RobotEngineManager.Instance.BlockPoolTracker;
      availableCubes = bpt.SortBlockStatesByRSSI(availableCubes);

      foreach (uint id in availableCubes) {
        if (!_AttemptedConnectionFactoryIds.Contains(id) && id != oldFactoryId) {
          selectedFactoryId = id;
          break;
        }
      }

      return selectedFactoryId;
    }

    private void HandleNumberAvailableObjectTypeChanged(ObjectType type) {
      if (type == _LightCubeType) {
        int availableObjectsOfType = 0;
        RobotEngineManager.Instance.BlockPoolTracker.ForEachObjectOfType(type, (BlockPoolData data) => {
          if (data.IsAvailable) {
            availableObjectsOfType++;
          }
        });

        _SwapCubeButton.gameObject.SetActive(availableObjectsOfType > 0);
      }
    }

    private void UpdateSwapCubeButton() {
      int numAvailableObjects = 0;
      RobotEngineManager.Instance.BlockPoolTracker.ForEachObjectOfType(_LightCubeType, (BlockPoolData data) => {
        if (data.IsAvailable) {
          numAvailableObjects++;
        }
      });
      _SwapCubeButton.gameObject.SetActive(numAvailableObjects > 0);
    }

    private void HandleFlashCubeButtonPressed() {
      if (_CurrentLightCube != null) {
        _CurrentRobot.FlashCurrentLightsState((uint)_CurrentLightCube.ID);
      }
    }
  }
}