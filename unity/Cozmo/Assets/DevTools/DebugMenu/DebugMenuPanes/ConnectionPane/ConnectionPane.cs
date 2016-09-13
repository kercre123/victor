using UnityEngine;
using System.Collections;
using Cozmo;

public class ConnectionPane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _DisconnectButton;
  [SerializeField]
  private UnityEngine.UI.Button _LowBatteryButton;

  // Use this for initialization
  void Start() {
    _DisconnectButton.onClick.AddListener(OnDisconnectButtonClicked);
    _LowBatteryButton.onClick.AddListener(OnLowBatteryButtonClicked);
  }

  private void OnDisconnectButtonClicked() {
    DebugMenuManager.Instance.CloseDebugMenuDialog();
    IntroManager.Instance.ForceBoot();
  }

  private void OnLowBatteryButtonClicked() {
#if ENABLE_DEBUG_PANEL
    DebugMenuManager.Instance.CloseDebugMenuDialog();
    PauseManager.Instance.FakeBatteryAlert();
#endif
  }
}
