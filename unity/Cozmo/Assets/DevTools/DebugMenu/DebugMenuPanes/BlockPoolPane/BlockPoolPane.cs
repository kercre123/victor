using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.UI;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public class BlockPoolPane : MonoBehaviour {
  [SerializeField]
  private GameObject _ButtonPrefab;

  [SerializeField]
  private Toggle _EnabledCheckbox;
  // Use this for initialization

  [SerializeField]
  private RectTransform _UIContainer;
  
  private U2G.BlockPoolEnabledMessage _BlockPoolEnabledMessage;
  private U2G.BlockSelectedMessage _BlockSelectedMessage;

  private class BlockData {
    public BlockData(bool is_enabled, sbyte signal_strength, Button btn) {
      this.IsEnabled = is_enabled;
      this.SignalStrength = signal_strength;
      this.Btn = btn;
    }

    public bool IsEnabled { get; set; }

    public sbyte SignalStrength { get; set; }

    public Button Btn { get; set; }
  }

  private Dictionary<uint,BlockData> _BlockStates;

  void Start() {

    _EnabledCheckbox.onValueChanged.AddListener(HandlePoolEnabledValueChanged);
    
    _BlockPoolEnabledMessage = new U2G.BlockPoolEnabledMessage();
    _BlockSelectedMessage = new U2G.BlockSelectedMessage();
    _BlockStates = new Dictionary<uint, BlockData>();

    RobotEngineManager.Instance.OnInitBlockPoolMsg += HandleInitBlockPool;
    
    // Gets back the InitBlockPool message to fill.
    RobotEngineManager.Instance.Message.GetBlockPoolMessage = new G2U.GetBlockPoolMessage();
    RobotEngineManager.Instance.SendMessage();
  }

  void HandleInitBlockPool(G2U.InitBlockPoolMessage initMsg) {

    // clear the lights we've turned blue to show connections.
    // Might stomp game setup but hopefully people are using this debug menu before playing.
    foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
      kvp.Value.SetLEDs(0, 0, 0, 0);
    }
    // Gets the previous ones enabled...
    _EnabledCheckbox.isOn = initMsg.blockPoolEnabled > 0;
    
    // Clear any old ones 
    foreach (Transform child in _UIContainer) {
      Destroy(child);
    }
 
    for (int i = 0; i < initMsg.blockData.Length; ++i) {
      // Never get an rssi value for things that were previously connected and won't be discovered 
      // if they connected to something else properly, so just indicate with a 0.
      AddButton(initMsg.blockData[i].id, initMsg.blockData[i].enabled, 0);
    }
    // The first one gets previous ones serialized that may or may exist, this message gets the one we see.
    RobotEngineManager.Instance.OnObjectDiscoveredMsg += HandleObjectDiscoveredMsg;
    RobotEngineManager.Instance.OnObjectUndiscoveredMsg -= HandleObjectUndiscoveredMsg;
    SendDiscoveredObjects(true);
  }

  void OnDestroy() {
    RobotEngineManager.Instance.OnInitBlockPoolMsg -= HandleInitBlockPool;
    RobotEngineManager.Instance.OnObjectDiscoveredMsg -= HandleObjectDiscoveredMsg;
    RobotEngineManager.Instance.OnObjectUndiscoveredMsg -= HandleObjectUndiscoveredMsg;
    SendDiscoveredObjects(false);
    // clear the lights we've turned blue to show connections
    foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
      kvp.Value.SetLEDs(0, 0, 0, 0);
    }
  }

  private void HandleObjectDiscoveredMsg(Anki.Cozmo.ObjectDiscovered objDiscoveredMsg) {
    AddButton(objDiscoveredMsg.factory_id, false, objDiscoveredMsg.rssi);
  }

  // haven't heard from this block in 10 seconds, remove it.
  private void HandleObjectUndiscoveredMsg(Anki.Cozmo.ObjectUndiscovered objUnDiscoveredMsg) {
    BlockPoolPane.BlockData data;
    if (_BlockStates.TryGetValue(objUnDiscoveredMsg.factory_id, out data)) {
      Destroy(data.Btn.gameObject);
      _BlockStates.Remove(objUnDiscoveredMsg.factory_id);
    }
  }

  private void HandleButtonClick(uint id) {
    BlockPoolPane.BlockData data;
    if (_BlockStates.TryGetValue(id, out data)) {
      bool is_enabled = !data.IsEnabled;
      data.IsEnabled = is_enabled;
      
      UpdateButton(id);
      _BlockSelectedMessage.factoryId = id;
      _BlockSelectedMessage.selected = is_enabled;
      RobotEngineManager.Instance.Message.BlockSelectedMessage = _BlockSelectedMessage;
      RobotEngineManager.Instance.SendMessage();
    }
  }

  private void SendDiscoveredObjects(bool enable) {
    // Will get a series of "Object Discovered" messages that represent blocks cozmo has "Heard" to connect to
    G2U.SendDiscoveredObjects msg = new G2U.SendDiscoveredObjects();
    msg.enable = enable;
    msg.robotID = (byte)RobotEngineManager.Instance.CurrentRobotID;
    RobotEngineManager.Instance.Message.SendDiscoveredObjects = msg;
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandlePoolEnabledValueChanged(bool val) {
    _BlockPoolEnabledMessage.enabled = val;
    RobotEngineManager.Instance.Message.BlockPoolEnabledMessage = _BlockPoolEnabledMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  private void AddButton(uint id, bool is_enabled, sbyte signal_strength) {
    BlockPoolPane.BlockData data;
    if (!_BlockStates.TryGetValue(id, out data)) {
      GameObject gameObject = UIManager.CreateUIElement(_ButtonPrefab, _UIContainer);
      Button btn = gameObject.GetComponent<Button>();
      data = new BlockPoolPane.BlockData(is_enabled, signal_strength, btn);
      _BlockStates.Add(id, data);
      UpdateButton(id);
      btn.onClick.AddListener(() => HandleButtonClick(id));
    }
    else if (data.SignalStrength != signal_strength) {
      // enabled is only changed form unity.
      data.SignalStrength = signal_strength;
      UpdateButton(id);
    }
  }

  private void UpdateButton(uint id) {
    BlockPoolPane.BlockData data;
    if (_BlockStates.TryGetValue(id, out data)) {
      Text txt = data.Btn.GetComponentInChildren<Text>();
      if (txt) {
        txt.text = "ID: " + id.ToString("X") + " \n " +
        "enabled: " + (data.IsEnabled ? "Y" : "N") + "\n" +
        "rssi: " + data.SignalStrength;
      }
      // Show all our enabled lights to blue so we can see what is currently connected
      foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(Color.blue.ToUInt(), Color.cyan.ToUInt());
      }
    }
  }
}
