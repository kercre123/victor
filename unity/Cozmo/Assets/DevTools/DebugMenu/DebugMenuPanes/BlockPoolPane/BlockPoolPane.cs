using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using U2G = Anki.Cozmo.ExternalInterface;

namespace Cozmo.BlockPool {
  public class BlockPoolPane : MonoBehaviour {

    private const int _DefaultRSSIFilter = 58;

    [SerializeField]
    private GameObject _ButtonPrefab;

    [SerializeField]
    private Toggle _EnabledCheckbox;

    [SerializeField]
    private RectTransform _AvailableScrollView;

    [SerializeField]
    private RectTransform _EnabledScrollView;

    [SerializeField]
    private InputField _FilterInput;
    private int _FilterRSSI = _DefaultRSSIFilter;

    [SerializeField]
    private Button _ResetButton;

    private BlockPoolTracker _BlockPoolTracker;

    private Dictionary<uint, Button> _FactoryIDToButton;

    void Start() {
      _FactoryIDToButton = new Dictionary<uint, Button>();

      _BlockPoolTracker = RobotEngineManager.Instance.BlockPoolTracker;

      // Disable free play lights
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayLightStates(false);
      }

      // Gets the previous ones enabled...
      _EnabledCheckbox.isOn = _BlockPoolTracker.AutoBlockPoolEnabled;

      // Clear any old ones 
      foreach (Transform child in _AvailableScrollView) {
        Destroy(child);
      }
        
      // Spawn buttons for existing block pool objects
      for (int i = 0; i < _BlockPoolTracker.Blocks.Count; i++) {
        AddOrUpdateButtonInternal(_BlockPoolTracker.Blocks[i], true);
      }

      // Listen for any changes
      _BlockPoolTracker.OnAutoBlockPoolEnabledChanged += HandleAutoBlockPoolEnabledChanged;
      _BlockPoolTracker.OnBlockDataUpdated += HandleBlockDataUpdated;
      _BlockPoolTracker.OnBlockDataUnavailable += HandleBlockDataUnavailable;
      _BlockPoolTracker.OnBlockDataConnectionChanged += HandleBlockDataConnectionChanged;
      _BlockPoolTracker.SendAvailableObjects(true, (byte)RobotEngineManager.Instance.CurrentRobotID);

      _FilterInput.text = _DefaultRSSIFilter.ToString();

      _FilterInput.onValueChanged.AddListener(HandleFilterUpdate);

      _EnabledCheckbox.onValueChanged.AddListener(HandlePoolEnabledValueChanged);

      _ResetButton.onClick.AddListener(HandleResetButtonClick);
    }

    private void OnDestroy() {
      _BlockPoolTracker.OnAutoBlockPoolEnabledChanged -= HandleAutoBlockPoolEnabledChanged;
      _BlockPoolTracker.OnBlockDataUpdated -= HandleBlockDataUpdated;
      _BlockPoolTracker.OnBlockDataUnavailable -= HandleBlockDataUnavailable;
      _BlockPoolTracker.OnBlockDataConnectionChanged -= HandleBlockDataConnectionChanged;
      _BlockPoolTracker.SendAvailableObjects(false, (byte)RobotEngineManager.Instance.CurrentRobotID);

      // clear the lights we've turned blue to show connections. Force the values right away since we are
      // being destroyed
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetEnableFreeplayLightStates(true);

        foreach (KeyValuePair<int, LightCube> kvp in robot.LightCubes) {
          kvp.Value.SetLEDs(0, 0, 0, 0);
        }

        if (robot.GetCharger() != null) {
          RobotEngineManager.Instance.CurrentRobot.GetCharger().SetLEDs(0, 0, 0, 0);
        }
      }
    }

    private void HandleFilterUpdate(string filter) {
      try {
        _FilterRSSI = int.Parse(filter);
      }
      catch (System.Exception) {
        // If the value is not a number, don't update the objects
        return;
      }

      foreach (BlockPoolData data in _BlockPoolTracker.Blocks) {
        Button button;
        if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
          // Don't filter out persistent or connected objects
          bool active = data.IsPersistent || data.IsConnected || data.RSSI <= _FilterRSSI;
          button.gameObject.SetActive(active);
        }
      }
    }

    private void HandlePoolEnabledValueChanged(bool val) {
      _BlockPoolTracker.EnableBlockPool(val);
    }

    private void HandleResetButtonClick() {
      _BlockPoolTracker.ResetBlockPool();
    }

    private void HandleButtonClick(uint factoryID) {
      BlockPoolData data = _BlockPoolTracker.GetObject(factoryID);
      if (data != null) {
        bool canToggleButton = true;
        if (!data.IsPersistent) {
          // If we are trying to pool a new object, make sure we haven't pooled one of the same type already
          bool pooledObjectOfSameType = false;
          _BlockPoolTracker.ForEachObjectOfType(data.ObjectType, (BlockPoolData objectData) => {
            if (objectData.IsPersistent && (data.ObjectType == objectData.ObjectType)) {
              pooledObjectOfSameType = true;
            }
          });

          canToggleButton = !pooledObjectOfSameType;
        }  

        if (canToggleButton) {
          _BlockPoolTracker.SetObjectInPool(data.FactoryID, !data.IsPersistent);

          Button button;
          if (_FactoryIDToButton.TryGetValue(factoryID, out button)) {
            DisableButton(data.FactoryID, GetButtonColor(data));
            UpdateButtonParent(data);
          }
        }
      }
    }

    private void HandleBlockDataConnectionChanged(BlockPoolData data) {
      bool isConnected = data.IsConnected;
      uint color = GetObjectColor(data);

      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        ObservedObject charger = robot.GetCharger();
        if ((data.ObjectType == ObjectType.Charger_Basic) && (charger != null) && (charger.FactoryID == data.FactoryID)) {
          uint period_ms = isConnected ? (uint)500 : 0;
          charger.SetLEDs(color, color, period_ms);
        }

        LightCube cube = robot.GetLightCubeWithFactoryID(data.FactoryID);
        if (cube != null) {
          uint period_ms = isConnected ? (uint)500 : 0;
          cube.SetLEDs(color, color, period_ms);
        }
      }

      UpdateButton(data);
      UpdateButtonParent(data);
      EnableButton(data.FactoryID, GetButtonColor(data));
    }

    private void HandleBlockDataUnavailable(BlockPoolData data) {
      if (data.IsPersistent) {
        EnableButton(data.FactoryID, GetButtonColor(data));
      }
      else {
        DisableButton(data.FactoryID, GetButtonColor(data));
      }
    }

    private void HandleAutoBlockPoolEnabledChanged(bool enabled) {
      _EnabledCheckbox.isOn = _BlockPoolTracker.AutoBlockPoolEnabled;
    }

    private void HandleBlockDataUpdated(BlockPoolData data, bool wasCreated) {
      AddOrUpdateButtonInternal(data, wasCreated);
      SortList();
    }

    private void AddOrUpdateButtonInternal(BlockPoolData data, bool wasCreated) {
      if (wasCreated) {
        AddButton(data);

        IRobot robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          LightCube cube = robot.GetLightCubeWithFactoryID(data.FactoryID);
          if (cube != null) {
            EnableButton(data.FactoryID, GetButtonColor(data));

            // Turn on the lights so we can see what is currently connected
            uint color = GetObjectColor(data);
            cube.SetLEDs(color, color);
          }
          else {
            ObservedObject observedObject = robot.GetObservedObjectWithFactoryID(data.FactoryID);
            if (observedObject != null) {
              EnableButton(data.FactoryID, GetButtonColor(data));

              // Turn on the lights to show that we are connected
              if (observedObject.ObjectType == ObjectType.Charger_Basic) {
                uint color = GetObjectColor(data);
                observedObject.SetLEDs(color, color);
              }
            }
            else {
              // Disable the button until we hear from the object if not enabled
              EnableButton(data.FactoryID, GetButtonColor(data));
            }
          }
        }
      }

      UpdateButton(data);
    }

    private void AddButton(BlockPoolData data) {
      if (!_FactoryIDToButton.ContainsKey(data.FactoryID)) {
        RectTransform parentTransform = data.IsPersistent ? _EnabledScrollView : _AvailableScrollView;
        GameObject newButton = UIManager.CreateUIElement(_ButtonPrefab, parentTransform);
        newButton.name = "object_" + data.FactoryID.ToString("X");

        Button button = newButton.GetComponent<Button>();
        button.onClick.AddListener(() => HandleButtonClick(data.FactoryID));
        _FactoryIDToButton.Add(data.FactoryID, button);
      }
    }

    private void UpdateButton(BlockPoolData data) {
      Button button;
      if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
        // Don't filter out enabled or persistent objects
        bool active = data.IsConnected || data.IsPersistent || data.RSSI <= _FilterRSSI;
        button.gameObject.SetActive(active);

        Text txt = button.GetComponentInChildren<Text>();
        if (txt) {
          txt.text = data.ObjectType + "\n" +
          "ID: " + data.FactoryID.ToString("X") + " \n " +
          "RSSI: " + data.RSSI;
        }
      }
    }

    private void EnableButton(uint factoryId, Color enabledColor) {
      Button button;
      if (_FactoryIDToButton.TryGetValue(factoryId, out button)) {
        button.interactable = true;
        button.image.color = enabledColor;
      }
    }

    private void DisableButton(uint factoryId, Color disabledColor) {
      Button button;
      if (_FactoryIDToButton.TryGetValue(factoryId, out button)) {
        button.interactable = false;
        button.image.color = disabledColor;
      }
    }

    private void SortList() {
      // Sort the list by RSSI
      _BlockPoolTracker.SortBlockStatesByRSSI();

      // Now reorder the UI elements. We can't reorder a transform's children but every child can be told its index
      BlockPoolData data;
      Button button;
      for (int i = 0; i < _BlockPoolTracker.Blocks.Count; i++) {
        data = _BlockPoolTracker.Blocks[i];
        if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
          // Don't reorder enabled objects
          if (!data.IsPersistent) {
            button.gameObject.transform.SetSiblingIndex(i);
          }
        }
      }
    }

    private void UpdateButtonParent(BlockPoolData data) {
      Button button;
      if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
        // Move the button to the correct list depending on whether it is enabled or not
        if (data.IsPersistent && (button.transform.parent != _EnabledScrollView)) {
          button.transform.SetParent(_EnabledScrollView);
        }
        else if (!data.IsPersistent && (button.transform.parent != _AvailableScrollView)) {
          button.transform.SetParent(_AvailableScrollView);
        }
      }
    }

    private Color GetButtonColor(BlockPoolData data) {
      switch (data.ConnectionState)
      {
      case BlockConnectionState.Connected:
        return data.IsPersistent ? Color.cyan : Color.green;

      case BlockConnectionState.ConnectInProgress:
      case BlockConnectionState.DisconnectInProgress:
        return Color.yellow;

      case BlockConnectionState.Available:
        return Color.white;

      case BlockConnectionState.Unavailable:
        return Color.grey;

      default:
        return Color.red;
      }
    }

    private uint GetObjectColor(BlockPoolData data) {
      if (data.IsConnected) {
        return data.IsPersistent ? Color.cyan.ToUInt() : Color.green.ToUInt();
      }
      else {
        return 0; 
      }
    }
  }
}