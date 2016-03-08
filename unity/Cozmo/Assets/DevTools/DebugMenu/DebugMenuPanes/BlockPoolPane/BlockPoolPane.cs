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
  private Dictionary<uint,bool> _BlockStates;

  void Start() {

    _EnabledCheckbox.onValueChanged.AddListener(HandleValueChanged);
    
    _BlockPoolEnabledMessage = new U2G.BlockPoolEnabledMessage();
    _BlockSelectedMessage = new U2G.BlockSelectedMessage();
    _BlockStates = new Dictionary<uint, bool>();

    RobotEngineManager.Instance.OnInitBlockPoolMsg += HandleInitBlockPool;
    
    // Gets back the InitBlockPool message to fill.
    RobotEngineManager.Instance.Message.GetBlockPoolMessage = new G2U.GetBlockPoolMessage();
    RobotEngineManager.Instance.SendMessage();
  }

  void OnDestroy() {
    RobotEngineManager.Instance.OnInitBlockPoolMsg -= HandleInitBlockPool;
    RobotEngineManager.Instance.OnObjectDiscoveredMsg -= HandleObjectDiscoveredMsg;
    ;
  }


  void HandleInitBlockPool(G2U.InitBlockPoolMessage initMsg) {
    // Gets the previous ones enabled...
    _EnabledCheckbox.isOn = initMsg.blockPoolEnabled > 0;
    
    // Clear any old ones 
    foreach (Transform child in _UIContainer) {
      Destroy(child);
    }
 
    for (int i = 0; i < initMsg.blockData.Length; ++i) {
      AddButton(initMsg.blockData[i].id, initMsg.blockData[i].enabled);
    }
    // The first one gets previous ones serialized that may or may exist, this message gets the one we see.
    RobotEngineManager.Instance.OnObjectDiscoveredMsg += HandleObjectDiscoveredMsg;
    // Will get a series of "Object Discovered" messages that represent blocks cozmo has "Heard" to connect to
    RobotEngineManager.Instance.Message.RequestDiscoveredObjects = new G2U.RequestDiscoveredObjects();
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandleObjectDiscoveredMsg(Anki.Cozmo.ObjectDiscovered objDiscoveredMsg) {
    AddButton(objDiscoveredMsg.factory_id, false);
  }

  private void AddButton(uint id, bool is_enabled) {

    GameObject gameObject = UIManager.CreateUIElement(_ButtonPrefab, _UIContainer);
    Button btn = gameObject.GetComponent<Button>();
    Text txt = btn.GetComponentInChildren<Text>();
      
    _BlockStates.Add(id, is_enabled);
    txt.text = id.ToString("X") + " , " + is_enabled;
    btn.onClick.AddListener(() => HandleButtonClick(id, txt));

  }

  private void HandleButtonClick(uint id, Text txt) {
    bool was_enabled = false;
    if (_BlockStates.TryGetValue(id, out was_enabled)) {
      bool is_enabled = !was_enabled;
      _BlockStates[id] = is_enabled;
      
      txt.text = id.ToString("X") + " , " + is_enabled;
      _BlockSelectedMessage.factoryId = id;
      _BlockSelectedMessage.selected = is_enabled;
      RobotEngineManager.Instance.Message.BlockSelectedMessage = _BlockSelectedMessage;
      RobotEngineManager.Instance.SendMessage();
    }
  }

  private void HandleValueChanged(bool val) {
    _BlockPoolEnabledMessage.enabled = val;
    RobotEngineManager.Instance.Message.BlockPoolEnabledMessage = _BlockPoolEnabledMessage;
    RobotEngineManager.Instance.SendMessage();
  }
}
