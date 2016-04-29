using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Debug;

// Attached to DebugMenu, active all the time to monitor connection health
// Automatically activates a display if we pass the warning threshold or can be activated from debug menu
public class LatencyCalculator : MonoBehaviour {

  [SerializeField]
  private GameObject _LatencyDisplayPrefab;

  private LatencyDisplay _LatencyDisplayInst;

  private bool _IsInitted;
  // Use this for initialization
  private void Start() {
    _IsInitted = false;
    DebugConsoleData.Instance.AddConsoleFunctionUnity("Show Latency Stats", "Network.Stats", this.HandleForceShowLatencyDisplay);
  }

  private void Update() {
    // Wait til we have a real connection
    if (!_IsInitted && RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.OnDebugLatencyMsg += HandleDebugLatencyMsg;
      RobotEngineManager.Instance.RobotConnected += HandleRobotConnected;
      _IsInitted = true;
    }
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.OnDebugLatencyMsg -= HandleDebugLatencyMsg;
    RobotEngineManager.Instance.RobotConnected -= HandleRobotConnected;
  }

  private void HandleDebugLatencyMsg(Anki.Cozmo.ExternalInterface.DebugLatencyMessage msg) {
    if (_LatencyDisplayInst) {
      _LatencyDisplayInst.HandleDebugLatencyMsg(msg);
    }
    else if (msg.averageLatency > LatencyDisplay.kMaxThresholdBeforeWarning_ms) {
      // Warning we are over the limit Warn people disconnects might happen.
      HandleForceShowLatencyDisplay();
    }
  }

  private void HandleRobotConnected(int id) {
    // Force init connection monitoring as long as we need this component.
    RobotEngineManager.Instance.SetDebugConsoleVar("NetConnStatsUpdate", "true");
  }
  // Hide happens just by hitting the x on the display.
  private void HandleForceShowLatencyDisplay(System.Object setvar = null) {
    if (_LatencyDisplayInst == null) {
      GameObject latencyDisplayInstance = GameObject.Instantiate(_LatencyDisplayPrefab.gameObject);
      latencyDisplayInstance.transform.SetParent(DebugMenuManager.Instance.DebugOverlayCanvas.transform, false);
      _LatencyDisplayInst = latencyDisplayInstance.GetComponent<LatencyDisplay>();
    }
  }

}
