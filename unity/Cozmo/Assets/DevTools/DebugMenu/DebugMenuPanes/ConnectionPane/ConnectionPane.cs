using UnityEngine;
using System.Collections;

public class ConnectionPane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _DisconnectButton;

  // Use this for initialization
  void Start() {
    _DisconnectButton.onClick.AddListener(OnDisconnectButtonClicked);
  }

  private void OnDisconnectButtonClicked() {
    DebugMenuManager.Instance.CloseDebugMenuDialog();
    IntroManager.Instance.ForceBoot();
  }
}
