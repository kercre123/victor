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
  public RectTransform _UIContainer;
  
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
  }

  void HandleInitBlockPool(G2U.InitBlockPoolMessage initMsg) {

    _EnabledCheckbox.isOn = initMsg.blockPoolEnabled > 0;
    
    // Clear any old ones 
    foreach (Transform child in _UIContainer) {
      Destroy(child);
    }
 
    for (int i = 0; i < initMsg.blockData.Length; ++i) {
      GameObject gameObject = UIManager.CreateUIElement(_ButtonPrefab, _UIContainer);
      Button btn = gameObject.GetComponent<Button>();
      Text txt = btn.GetComponentInChildren<Text>();
      uint id = initMsg.blockData[i].id;
      bool is_enabled = initMsg.blockData[i].enabled;
      _BlockStates.Add(id, is_enabled);
      txt.text = id.ToString("X") + " , " + is_enabled;
      btn.onClick.AddListener(() => HandleButtonClick(id, txt));
    }

  }

  private void HandleButtonClick(uint id, Text txt) {
    bool was_enabled = false;
    if (_BlockStates.TryGetValue(id, out was_enabled)) {
      bool is_enabled = !was_enabled;
      _BlockStates[id] = is_enabled;
      
      txt.text = id.ToString("X") + " , " + is_enabled;
      _BlockSelectedMessage.blockId = id;
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
