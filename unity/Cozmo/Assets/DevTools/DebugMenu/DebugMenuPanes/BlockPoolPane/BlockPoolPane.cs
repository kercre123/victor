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

      // Might stomp game setup but hopefully people are using this debug menu before playing.
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
          kvp.Value.SetLEDs(0, 0, 0, 0);
        }
      }

      // Gets the previous ones enabled...
      _EnabledCheckbox.isOn = _BlockPoolTracker.AutoBlockPoolEnabled;

      // Clear any old ones 
      foreach (Transform child in _AvailableScrollView) {
        Destroy(child);
      }

      // Spawn buttons for existing block pool objects
      for (int i = 0; i < _BlockPoolTracker.BlockStates.Count; i++) {
        AddOrUpdateButtonInternal(_BlockPoolTracker.BlockStates[i], true);
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
        foreach (KeyValuePair<int, LightCube> kvp in robot.LightCubes) {
          kvp.Value.SetLEDs(0, 0, 0, 0);
          kvp.Value.SetAllLEDs();
          robot.SetEnableFreeplayLightStates(true, kvp.Value.ID);
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

      foreach (BlockPoolData data in _BlockPoolTracker.BlockStates) {
        // Don't filter out enabled objects
        bool active = data.IsConnected || data.RSSI <= _FilterRSSI;
        Button button;
        if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
          button.gameObject.SetActive(active);
        }
      }
    }

    private void HandlePoolEnabledValueChanged(bool val) {
      _BlockPoolTracker.EnableBlockPool(val);
    }

    private void HandleResetButtonClick() {
      _BlockPoolTracker.ResetBlockPool();

      // Disable all the buttons
      foreach (BlockPoolData data in _BlockPoolTracker.BlockStates) {
        if (data.IsConnected) {
          EnableButton(data.FactoryID, Color.white);
          UpdateButtonParent(data);
        }
      }
    }

    private void HandleButtonClick(uint factoryID) {
      BlockPoolData data = _BlockPoolTracker.ToggleObjectInPool(factoryID);
      if (data != null) {
        Button button;
        if (_FactoryIDToButton.TryGetValue(factoryID, out button)) {
          // Turn the button to a different color to signal we are trying to connect/disconnect from/to the object
          // Unless the button is already grey meaning we not hearing from the cube
          Color newColor = (button.image.color != Color.grey) ? Color.yellow : Color.grey;
          DisableButton(data.FactoryID, newColor);

          UpdateButtonParent(data);
        }
      }
    }

    private void HandleBlockDataConnectionChanged(BlockPoolData data) {
      bool isConnected = data.IsConnected;
      ObservedObject charger = RobotEngineManager.Instance.CurrentRobot.GetCharger();
      if ((data.ObjectType == ObjectType.Charger_Basic) && (charger != null)) {
        uint onColor = isConnected ? Color.blue.ToUInt() : 0;
        uint offColor = isConnected ? Color.cyan.ToUInt() : 0;
        uint period_ms = isConnected ? (uint)500 : 0;
        charger.SetLEDs(onColor, offColor, period_ms);
      }

      LightCube cube = RobotEngineManager.Instance.CurrentRobot.GetLightCubeWithFactoryID(data.FactoryID);
      if (cube != null) {
        uint onColor = isConnected ? Color.blue.ToUInt() : 0;
        uint offColor = isConnected ? Color.cyan.ToUInt() : 0;
        uint period_ms = isConnected ? (uint)500 : 0;
        cube.SetLEDs(onColor, offColor, period_ms);
      }

      UpdateButton(data);
      UpdateButtonParent(data);
      if (data.IsConnected) {
        EnableButton(data.FactoryID, Color.cyan);
      }
      else {
        EnableButton(data.FactoryID, Color.white);
      }
    }

    private void HandleBlockDataUnavailable(BlockPoolData data) {
      if (data.IsConnected) {
        EnableButton(data.FactoryID, Color.grey);
      }
      else {
        DisableButton(data.FactoryID, Color.grey);
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
      }

      UpdateButton(data);

      // TODO Fix this part
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        LightCube cube = robot.GetLightCubeWithFactoryID(data.FactoryID);
        if (cube != null) {
          EnableButton(data.FactoryID, (data.IsConnected ? Color.cyan : Color.white));

          // Show all our enabled lights to blue so we can see what is currently connected
          robot.SetEnableFreeplayLightStates(false, cube.ID);
          cube.SetLEDs(Color.blue.ToUInt(), Color.cyan.ToUInt());
        }
        else {
          ObservedObject observedObject = robot.GetObservedObjectWithFactoryID(data.FactoryID);
          if (observedObject != null) {
            EnableButton(data.FactoryID, (data.IsConnected ? Color.cyan : Color.white));

            // Turn on the lights to show that we are connected
            if (observedObject.ObjectType == ObjectType.Charger_Basic) {
              observedObject.SetLEDs(Color.blue.ToUInt(), Color.cyan.ToUInt());
            }
          }
          else {
            // Disable the button until we hear from the object if not enabled
            if (data.IsConnected) {
              EnableButton(data.FactoryID, Color.grey);
            }
            else {
              EnableButton(data.FactoryID, Color.white);
            }
          }
        }
      }
    }

    private void AddButton(BlockPoolData data) {
      if (!_FactoryIDToButton.ContainsKey(data.FactoryID)) {
        RectTransform parentTransform = data.IsConnected ? _EnabledScrollView : _AvailableScrollView;
        GameObject newButton = UIManager.CreateUIElement(_ButtonPrefab, parentTransform);
        newButton.name = "object_" + data.FactoryID.ToString("X");

        Button button = newButton.GetComponent<Button>();
        button.onClick.AddListener(() => HandleButtonClick(data.FactoryID));
        _FactoryIDToButton.Add(data.FactoryID, button);
      }
    }

    private void UpdateButton(BlockPoolData data) {
      // Don't filter out enabled objects
      bool active = data.IsConnected || data.RSSI <= _FilterRSSI;

      Button button;
      if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
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
      for (int i = 0; i < _BlockPoolTracker.BlockStates.Count; i++) {
        data = _BlockPoolTracker.BlockStates[i];
        if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
          // Don't reorder enabled objects
          if (!data.IsConnected) {
            button.gameObject.transform.SetSiblingIndex(i);
          }
        }
      }
    }

    void UpdateButtonParent(BlockPoolData data) {
      Button button;
      if (_FactoryIDToButton.TryGetValue(data.FactoryID, out button)) {
        // Move the button to the correct list depending on whether it is enabled or not
        if (data.IsConnected && (button.transform.parent != _EnabledScrollView)) {
          button.transform.SetParent(_EnabledScrollView);
        }
        else if (!data.IsConnected && (button.transform.parent != _AvailableScrollView)) {
          button.transform.SetParent(_AvailableScrollView);
        }
      }
    }
  }
}