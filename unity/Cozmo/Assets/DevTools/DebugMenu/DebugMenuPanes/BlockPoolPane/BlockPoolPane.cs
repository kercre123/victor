using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.UI;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public class BlockPoolPane : MonoBehaviour {

  private const int _DefaultRSSIFilter = 60;

  [SerializeField]
  private GameObject _ButtonPrefab;

  [SerializeField]
  private Toggle _EnabledCheckbox;

  [SerializeField]
  private Toggle _UpdateCheckbox;

  [SerializeField]
  private RectTransform _UIContainer;

  [SerializeField]
  private InputField _FilterInput;
  private int _FilterRSSI = _DefaultRSSIFilter;
  
  private U2G.BlockPoolEnabledMessage _BlockPoolEnabledMessage;
  private U2G.BlockSelectedMessage _BlockSelectedMessage;

  private class BlockData {
    public BlockData(Anki.Cozmo.ObjectType type, uint id, bool is_enabled, sbyte rssi, Button button) {
      this.ObjectType = type;
      this.Id = id;
      this.IsEnabled = is_enabled;
      this.RSSI = rssi;
      this.BlockButton = button;
    }

    public Anki.Cozmo.ObjectType ObjectType { get; set; }

    public uint Id { get; set; }

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
    _UpdateCheckbox.onValueChanged.AddListener(HandleUpdateListValueChanged);

    _BlockPoolEnabledMessage = new U2G.BlockPoolEnabledMessage();
    _BlockSelectedMessage = new U2G.BlockSelectedMessage();
    _BlockStatesById = new Dictionary<uint, BlockData>();
    _BlockStates = new List<BlockData>();

    RobotEngineManager.Instance.OnInitBlockPoolMsg += HandleInitBlockPool;
    
    // Gets back the InitBlockPool message to fill.
    RobotEngineManager.Instance.Message.GetBlockPoolMessage = new G2U.GetBlockPoolMessage();
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandleFilterUpdate(string filter) {
    try  {
      _FilterRSSI = int.Parse(filter);
    } catch (System.Exception) {
      // If the value is not a number, don't update the objects
      return;
    }

    foreach (KeyValuePair<uint, BlockData> kvp in _BlockStatesById) {
      if (kvp.Value.RSSI < _FilterRSSI) {
        kvp.Value.BlockButton.gameObject.SetActive(true);
      }
      else {
        kvp.Value.BlockButton.gameObject.SetActive(false);
      }
    }
  }

  private void HandleInitBlockPool(G2U.InitBlockPoolMessage initMsg) {

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
      AddButton(initMsg.blockData[i].factory_id, Anki.Cozmo.ObjectType.Unknown, initMsg.blockData[i].enabled, 0);
    }
    // The first one gets previous ones serialized that may or may exist, this message gets the one we see.
    RobotEngineManager.Instance.OnObjectAvailableMsg += HandleObjectAvailableMsg;
    RobotEngineManager.Instance.OnObjectUnavailableMsg += HandleObjectUnavailableMsg;
    SendAvailableObjects(true);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.OnInitBlockPoolMsg -= HandleInitBlockPool;
    RobotEngineManager.Instance.OnObjectAvailableMsg -= HandleObjectAvailableMsg;
    RobotEngineManager.Instance.OnObjectUnavailableMsg -= HandleObjectUnavailableMsg;
    SendAvailableObjects(false);
    // clear the lights we've turned blue to show connections
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(0, 0, 0, 0);
      }
    }

  }

  private void HandleObjectAvailableMsg(Anki.Cozmo.ExternalInterface.ObjectAvailable objAvailableMsg) {
    switch (objAvailableMsg.objectType) {
    case Anki.Cozmo.ObjectType.Block_LIGHTCUBE1:
    case Anki.Cozmo.ObjectType.Block_LIGHTCUBE2:
    case Anki.Cozmo.ObjectType.Block_LIGHTCUBE3:
    case Anki.Cozmo.ObjectType.Charger_Basic:
      AddButton(objAvailableMsg.factory_id, objAvailableMsg.objectType, false, objAvailableMsg.rssi);
      break;
    default:
      break;
    }
  }

  // haven't heard from this block in 10 seconds, remove it.
  private void HandleObjectUnavailableMsg(Anki.Cozmo.ExternalInterface.ObjectUnavailable objUnAvailableMsg) {
    BlockPoolPane.BlockData data;
    if (_BlockStatesById.TryGetValue(objUnAvailableMsg.factory_id, out data)) {
      Destroy(data.BlockButton.gameObject);

      _BlockStatesById.Remove(objUnAvailableMsg.factory_id);

      int index = _BlockStates.FindIndex((BlockData obj) => { 
        return objUnAvailableMsg.factory_id == obj.Id; 
      });
      _BlockStates.RemoveAt(index);
    }
  }

  private void HandleButtonClick(uint id) {
    BlockPoolPane.BlockData data;
    if (_BlockStatesById.TryGetValue(id, out data)) {
      bool is_enabled = !data.IsEnabled;
      data.IsEnabled = is_enabled;
      UpdateButton(data);
      _BlockSelectedMessage.factoryId = id;
      _BlockSelectedMessage.selected = is_enabled;
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

  private void HandleUpdateListValueChanged(bool val) {
    SendAvailableObjects(val);
  }

  private void AddButton(uint id, Anki.Cozmo.ObjectType type, bool is_enabled, sbyte signal_strength) {
    BlockPoolPane.BlockData data;
    if (!_BlockStatesById.TryGetValue(id, out data)) {
      GameObject gameObject = UIManager.CreateUIElement(_ButtonPrefab, _UIContainer);
      gameObject.name = "block_" + id.ToString("X");
      Button button = gameObject.GetComponent<Button>();

      data = new BlockPoolPane.BlockData(type, id, is_enabled, signal_strength, button);
      _BlockStatesById.Add(id, data);
      _BlockStates.Add(data);

      UpdateButton(data);
      button.onClick.AddListener(() => HandleButtonClick(id));
    }
    else if (data.RSSI != signal_strength || data.ObjectType != type) {
      // enabled is only changed form unity.
      data.RSSI = signal_strength;
      data.ObjectType = type;
      UpdateButton(data);
    }

    SortList();
  }

  private void UpdateButton(BlockData data) {
    if (data.RSSI < _FilterRSSI) {
      data.BlockButton.gameObject.SetActive(true);
    }
    else {
      data.BlockButton.gameObject.SetActive(false);
    }
    Text txt = data.BlockButton.GetComponentInChildren<Text>();
    if (txt) {
      txt.text = "ID: " + data.Id.ToString("X") + " \n " +
      "type: " + data.ObjectType + "\n" +
      "enabled: " + (data.IsEnabled ? "Y" : "N") + "\n" +
      "rssi: " + data.RSSI;
    }
    // Show all our enabled lights to blue so we can see what is currently connected
    foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
      kvp.Value.SetLEDs(Color.blue.ToUInt(), Color.cyan.ToUInt());
    }
  }

  private void SortList() {
    // Sort the list by RSSI
    _BlockStates.Sort((BlockData x, BlockData y) => {
      return x.RSSI.CompareTo(y.RSSI);
    });

    // Now reorder the UI elements. We reorder a transform children but every child can be told its index
    for (int i = 0; i < _BlockStates.Count; i++) {
      BlockData bd = _BlockStates[i];
      bd.BlockButton.gameObject.transform.SetSiblingIndex(i);
    }
  }
}
