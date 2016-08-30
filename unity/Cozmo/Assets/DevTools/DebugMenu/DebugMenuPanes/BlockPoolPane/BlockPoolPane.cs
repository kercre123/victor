using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.UI;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;
using Anki.Cozmo;

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

  private U2G.BlockPoolEnabledMessage _BlockPoolEnabledMessage;
  private U2G.BlockSelectedMessage _BlockSelectedMessage;

  private const float kUpdateDelay_Secs = 1;
  private float _LastUpdateTime;

  private class BlockData {
    public BlockData(Anki.Cozmo.ObjectType type, uint factoryID, bool is_enabled, sbyte rssi, Button button) {
      this.ObjectType = type;
      this.FactoryID = factoryID;
      this.IsEnabled = is_enabled;
      this.RSSI = rssi;
      this.BlockButton = button;
    }

    public Anki.Cozmo.ObjectType ObjectType { get; set; }

    public uint FactoryID { get; set; }

    public bool IsEnabled { get; set; }

    public sbyte RSSI { get; set; }

    public Button BlockButton { get; set; }
  }

  private Dictionary<uint, BlockData> _BlockStatesById;
  private List<BlockData> _BlockStates;

  void Start() {

    _FilterInput.text = _DefaultRSSIFilter.ToString();

    _FilterInput.onValueChanged.AddListener(HandleFilterUpdate);

    _EnabledCheckbox.onValueChanged.AddListener(HandlePoolEnabledValueChanged);

    _ResetButton.onClick.AddListener(HandleResetButtonClick);

    _BlockPoolEnabledMessage = new U2G.BlockPoolEnabledMessage();
    _BlockSelectedMessage = new U2G.BlockSelectedMessage();
    _BlockStatesById = new Dictionary<uint, BlockData>();
    _BlockStates = new List<BlockData>();

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.InitBlockPoolMessage>(HandleInitBlockPool);

    // Gets back the InitBlockPool message to fill.
    RobotEngineManager.Instance.Message.GetBlockPoolMessage = new G2U.GetBlockPoolMessage();
    RobotEngineManager.Instance.SendMessage();
  }

  void Update() {
    if (Time.time - _LastUpdateTime > kUpdateDelay_Secs) {
      // Check if there is any button that should be updated. This solves issues when blockpool is opened and
      // the robot class doesn't have the information about all the cubes yet.
      foreach (var blockState in _BlockStates) {
        if ((blockState.ObjectType == ObjectType.Invalid) || (blockState.ObjectType == ObjectType.Unknown)) {
          LightCube lc = RobotEngineManager.Instance.CurrentRobot.GetLightCubeWithFactoryID(blockState.FactoryID);
          if (lc != null) {
            blockState.ObjectType = lc.ObjectType;
            UpdateButton(blockState);
          }
          else {
            ObservedObject oo = RobotEngineManager.Instance.CurrentRobot.GetObservedObjectWithFactoryID(blockState.FactoryID);
            if (oo != null) {
              blockState.ObjectType = oo.ObjectType;
              UpdateButton(blockState);
            }
          }
        }
      }

      _LastUpdateTime = Time.time;
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

    foreach (KeyValuePair<uint, BlockData> kvp in _BlockStatesById) {
      // Don't filter out enabled objects
      bool active = kvp.Value.IsEnabled || kvp.Value.RSSI <= _FilterRSSI;
      kvp.Value.BlockButton.gameObject.SetActive(active);
    }
  }

  private void HandleInitBlockPool(Anki.Cozmo.ExternalInterface.InitBlockPoolMessage initMsg) {
    // Might stomp game setup but hopefully people are using this debug menu before playing.
    foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
      kvp.Value.SetLEDs(0, 0, 0, 0);
    }

    // Gets the previous ones enabled...
    _EnabledCheckbox.isOn = initMsg.blockPoolEnabled > 0;

    // Clear any old ones 
    foreach (Transform child in _AvailableScrollView) {
      Destroy(child);
    }

    for (int i = 0; i < initMsg.blockData.Length; ++i) {
      // See if we already know about the object. This happens when the blockpool pane is opened after the robot
      // has already connected to some objects.
      LightCube lc = RobotEngineManager.Instance.CurrentRobot.GetLightCubeWithFactoryID(initMsg.blockData[i].factory_id);
      if (lc != null) {
        BlockData data = AddButton(initMsg.blockData[i].factory_id, lc.ObjectType, initMsg.blockData[i].enabled, 0);
        EnableButton(data.BlockButton, Color.cyan);

        // Show all our enabled lights to blue so we can see what is currently connected
        lc.SetLEDs(Color.blue.ToUInt(), Color.cyan.ToUInt());
      }
      else {
        ObservedObject oo = RobotEngineManager.Instance.CurrentRobot.GetObservedObjectWithFactoryID(initMsg.blockData[i].factory_id);
        if (oo != null) {
          // Create the button with the information that we know
          BlockData data = AddButton(initMsg.blockData[i].factory_id, oo.ObjectType, initMsg.blockData[i].enabled, 0);
          EnableButton(data.BlockButton, Color.cyan);

          // Turn on the lights to show that we are connected
          if (oo.ObjectType == ObjectType.Charger_Basic) {
            oo.SetLEDs(Color.blue.ToUInt(), Color.cyan.ToUInt());
          }
        }
        else {
          // Create the button with the information that we know and disable the button until we hear from the object
          BlockData data = AddButton(initMsg.blockData[i].factory_id, ObjectType.Unknown, initMsg.blockData[i].enabled, 0);
          if (initMsg.blockData[i].enabled) {
            EnableButton(data.BlockButton, Color.grey);
          } else {
            DisableButton(data.BlockButton, Color.grey);
          }
        }
      }
    }

    // The first one gets previous ones serialized that may or may exist, this message gets the one we see.
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ObjectConnectionState>(HandleObjectConnectionState);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.ObjectAvailable>(HandleObjectAvailableMsg);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.ObjectUnavailable>(HandleObjectUnavailableMsg);
    SendAvailableObjects(true);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.InitBlockPoolMessage>(HandleInitBlockPool);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ObjectConnectionState>(HandleObjectConnectionState);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ObjectAvailable>(HandleObjectAvailableMsg);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ObjectUnavailable>(HandleObjectUnavailableMsg);
    SendAvailableObjects(false);

    // clear the lights we've turned blue to show connections. Force the values right away since we are
    // being destroyed
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(0, 0, 0, 0);
        kvp.Value.SetAllLEDs();
      }

      if (RobotEngineManager.Instance.CurrentRobot.GetCharger() != null) {
        RobotEngineManager.Instance.CurrentRobot.GetCharger().SetLEDs(0, 0, 0, 0);
      }
    }
  }

  private void HandleObjectConnectionState(Anki.Cozmo.ObjectConnectionState message) {
    // Turn on or off the lights depending on whether we are connecting or disconnecting
    if ((message.device_type == ActiveObjectType.OBJECT_CHARGER) && (RobotEngineManager.Instance.CurrentRobot.GetCharger() != null)) {
      uint onColor = message.connected ? Color.blue.ToUInt() : 0;
      uint offColor = message.connected ? Color.cyan.ToUInt() : 0;
      uint period_ms = message.connected ? uint.MaxValue : 0;
      RobotEngineManager.Instance.CurrentRobot.GetCharger().SetLEDs(onColor, offColor, period_ms);
    }

    LightCube lc = RobotEngineManager.Instance.CurrentRobot.GetLightCubeWithFactoryID(message.factoryID);
    if (lc != null) {
      uint onColor = message.connected ? Color.blue.ToUInt() : 0;
      uint offColor = message.connected ? Color.cyan.ToUInt() : 0;
      uint period_ms = message.connected ? uint.MaxValue : 0;
      lc.SetLEDs(onColor, offColor, period_ms);
    }

    // Update or create the button for the object
    BlockPoolPane.BlockData data;
    if (_BlockStatesById.TryGetValue(message.factoryID, out data)) {
      data.IsEnabled = message.connected;
      UpdateButton(data);
      UpdateButtonParent(data);

      if (message.connected) {
        EnableButton(data.BlockButton, Color.cyan);
      } 
      else {
        if (data.IsEnabled) {
          EnableButton(data.BlockButton, Color.grey);
        }
        else {
          DisableButton(data.BlockButton, Color.grey);
        }
      }
    }
  }

  private void HandleObjectAvailableMsg(Anki.Cozmo.ExternalInterface.ObjectAvailable objAvailableMsg) {
    switch (objAvailableMsg.objectType) {
    case Anki.Cozmo.ObjectType.Block_LIGHTCUBE1:
    case Anki.Cozmo.ObjectType.Block_LIGHTCUBE2:
    case Anki.Cozmo.ObjectType.Block_LIGHTCUBE3:
    case Anki.Cozmo.ObjectType.Charger_Basic:
      BlockData data = AddButton(objAvailableMsg.factory_id, objAvailableMsg.objectType, false, objAvailableMsg.rssi);
      EnableButton(data.BlockButton, (data.IsEnabled ? Color.cyan : Color.white));
      break;

    default:
      break;
    }
  }

  private void HandleObjectUnavailableMsg(Anki.Cozmo.ExternalInterface.ObjectUnavailable objUnAvailableMsg) {
    BlockPoolPane.BlockData data;
    if (_BlockStatesById.TryGetValue(objUnAvailableMsg.factory_id, out data)) {
      if (data.IsEnabled) {
        EnableButton(data.BlockButton, Color.grey);
      }
      else {
        DisableButton(data.BlockButton, Color.grey);
      }
    }
  }

  private void HandleResetButtonClick() {

    // Disable all the buttons
    foreach (KeyValuePair<uint, BlockData> kvp in _BlockStatesById) {
      if (kvp.Value.IsEnabled) {
        kvp.Value.IsEnabled = false;
        EnableButton(kvp.Value.BlockButton, Color.white);
        UpdateButtonParent(kvp.Value);
      }
    }

    RobotEngineManager.Instance.Message.BlockPoolResetMessage = new U2G.BlockPoolResetMessage();
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandleButtonClick(uint factoryID) {
    BlockPoolPane.BlockData data;
    if (_BlockStatesById.TryGetValue(factoryID, out data)) {
      data.IsEnabled = !data.IsEnabled;

      // Turn the button to a different color to signal we are trying to connect/disconnect from/to the object
      // Unless the button is already grey meaning we not hearing from the cube
      Color newColor = (data.BlockButton.image.color != Color.grey) ? Color.yellow : Color.grey;
      DisableButton(data.BlockButton, newColor);
    
      UpdateButtonParent(data);

      // Send the message to engine
      _BlockSelectedMessage.factoryId = factoryID;
      _BlockSelectedMessage.objectType = data.ObjectType;
      _BlockSelectedMessage.selected = data.IsEnabled;
      RobotEngineManager.Instance.Message.BlockSelectedMessage = _BlockSelectedMessage;
      RobotEngineManager.Instance.SendMessage();
    }
  }

  private void SendAvailableObjects(bool enable) {
    // Will get a series of "Object Available" messages that represent blocks cozmo has "Heard" to connect to
    G2U.SendAvailableObjects msg = new G2U.SendAvailableObjects();
    msg.enable = enable;
    msg.robotID = (byte)RobotEngineManager.Instance.CurrentRobotID;
    RobotEngineManager.Instance.Message.SendAvailableObjects = msg;
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandlePoolEnabledValueChanged(bool val) {
    _BlockPoolEnabledMessage.enabled = val;
    RobotEngineManager.Instance.Message.BlockPoolEnabledMessage = _BlockPoolEnabledMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  private BlockData AddButton(uint factoryID, Anki.Cozmo.ObjectType type, bool is_enabled, sbyte signal_strength) {
    BlockPoolPane.BlockData data;
    if (!_BlockStatesById.TryGetValue(factoryID, out data)) {
      RectTransform parentTransform = is_enabled ? _EnabledScrollView : _AvailableScrollView;
      GameObject gameObject = UIManager.CreateUIElement(_ButtonPrefab, parentTransform);
      gameObject.name = "object_" + factoryID.ToString("X");

      Button button = gameObject.GetComponent<Button>();
      button.onClick.AddListener(() => HandleButtonClick(factoryID));

      data = new BlockPoolPane.BlockData(type, factoryID, is_enabled, signal_strength, button);
      _BlockStatesById.Add(factoryID, data);
      _BlockStates.Add(data);

      UpdateButton(data);
      SortList();
    }
    else {
      if (data.RSSI != signal_strength || data.ObjectType != type) {
        data.RSSI = signal_strength;
        data.ObjectType = type;
        UpdateButton(data);
        SortList();
      } 
    }

    return data;
  }

  private void UpdateButton(BlockData data) {
    // Don't filter out enabled objects
    bool active = data.IsEnabled || data.RSSI <= _FilterRSSI;
    data.BlockButton.gameObject.SetActive(active);

    Text txt = data.BlockButton.GetComponentInChildren<Text>();
    if (txt) {
      txt.text = data.ObjectType + "\n" +
      "ID: " + data.FactoryID.ToString("X") + " \n " +
      "RSSI: " + data.RSSI;
    }
  }

  private void SortList() {
    // Sort the list by RSSI
    _BlockStates.Sort((BlockData x, BlockData y) => {
      // In order to have a more stable sort that doesn't continuously switch around 
      // two elements with the same RSSI, if the RSSI are equal then we sort based
      // on the ID
      int comparison = x.RSSI.CompareTo(y.RSSI);
      return (comparison != 0) ? comparison : x.FactoryID.CompareTo(y.FactoryID);
    });

    // Now reorder the UI elements. We can't reorder a transform's children but every child can be told its index
    for (int i = 0; i < _BlockStates.Count; i++) {
      BlockData data = _BlockStates[i];

      // Don't reorder enabled objects
      if (!data.IsEnabled) {
        data.BlockButton.gameObject.transform.SetSiblingIndex(i);
      }
    }
  }

  private void EnableButton(Button button, Color enabledColor) {
    button.interactable = true;
    button.image.color = enabledColor;
  }

  private void DisableButton(Button button, Color disabledColor) {
    button.interactable = false;
    button.image.color = disabledColor;
  }

  void UpdateButtonParent(BlockData data) {
    // Move the button to the correct list depending on whether it is enabled or not
    if (data.IsEnabled && (data.BlockButton.transform.parent != _EnabledScrollView)) {
      data.BlockButton.transform.SetParent(_EnabledScrollView);
    }
    else if (!data.IsEnabled && (data.BlockButton.transform.parent != _AvailableScrollView)) {
        data.BlockButton.transform.SetParent(_AvailableScrollView);
    }
  }

}
