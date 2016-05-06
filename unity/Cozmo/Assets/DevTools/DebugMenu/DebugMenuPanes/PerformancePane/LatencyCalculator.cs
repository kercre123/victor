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
  private bool _Enabled = true;

  // Use this for initialization
  private void Start() {
    _IsInitted = false;
  }

  public void EnableLatencyPopup(bool enable) {
    _Enabled = enable;
    if (_Enabled == false && _LatencyDisplayInst != null) {
      GameObject.Destroy(_LatencyDisplayInst.gameObject);
    }
  }

  private void Update() {
    if (_Enabled == false) {
      return;
    }
    // Wait til we have a real connection
    if (!_IsInitted && RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.OnDebugLatencyMsg += HandleDebugLatencyMsg;
      RobotEngineManager.Instance.RobotConnected += HandleRobotConnected;
      _IsInitted = true;
      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.LatencyDisplayEnabled) {
        ShowLatencyDisplay();
      }
    }
  }

  private void OnDestroy() {
    if (_IsInitted) {
      RobotEngineManager.Instance.OnDebugLatencyMsg -= HandleDebugLatencyMsg;
      RobotEngineManager.Instance.RobotConnected -= HandleRobotConnected;
    }
  }

  private void HandleDebugLatencyMsg(Anki.Cozmo.ExternalInterface.DebugLatencyMessage msg) {
    if (_Enabled) {
      if (_LatencyDisplayInst) {
        _LatencyDisplayInst.HandleDebugLatencyMsg(msg);
      }
      else if (msg.averageLatency > LatencyDisplay.kMaxThresholdBeforeWarning_ms) {
        // Warning we are over the limit Warn people disconnects might happen.
        ShowLatencyDisplay();
      }
    }

  }

  private void HandleRobotConnected(int id) {
    // Force init connection monitoring as long as we need this component.
    RobotEngineManager.Instance.SetDebugConsoleVar("NetConnStatsUpdate", "true");
  }

  private void ShowLatencyDisplay() {
    if (_Enabled) {
      if (_LatencyDisplayInst == null) {
        GameObject latencyDisplayInstance = GameObject.Instantiate(_LatencyDisplayPrefab.gameObject);
        latencyDisplayInstance.transform.SetParent(DebugMenuManager.Instance.DebugOverlayCanvas.transform, false);
        _LatencyDisplayInst = latencyDisplayInstance.GetComponent<LatencyDisplay>();
      }
    }
  }

}
