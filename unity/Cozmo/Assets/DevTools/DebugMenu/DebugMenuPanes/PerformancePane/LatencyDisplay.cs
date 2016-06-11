using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Debug;

// This class is the actual prefab that displays data.
// It doesn't exist when the display isn't active.
public class LatencyDisplay : MonoBehaviour {
  [SerializeField]
  private Text _WarningLabel;

  [SerializeField]
  private Text _LatencyAvgLabel;
  [SerializeField]
  private Text _LatencyMaxLabel;
  [SerializeField]
  private Text _LatencyMinLabel;

  [SerializeField]
  private Button _CloseButton;

  public const float kMaxThresholdBeforeWarning_ms = 60.0f;

  // Use this for initialization
  private void Start() {
    _CloseButton.onClick.AddListener(HandleCloseButtonClick);
    UpdateUI(0.0f, 0.0f, 0.0f);
  }

  private void OnDestroy() {
    _CloseButton.onClick.RemoveListener(HandleCloseButtonClick);
  }

  public void HandleDebugLatencyMsg(Anki.Cozmo.ExternalInterface.DebugLatencyMessage msg) {
    UpdateUI(msg.wifiLatency.avgTime_ms, msg.wifiLatency.minTime_ms, msg.wifiLatency.maxTime_ms);
  }

  public void SaveEnabled(bool isEnabled) {
    DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.LatencyDisplayEnabled = isEnabled;
    DataPersistence.DataPersistenceManager.Instance.Save();
  }

  private void UpdateUI(float avg, float min, float max) {
    _LatencyAvgLabel.text = "Avg: " + avg.ToString("F2");
    _LatencyMaxLabel.text = "Max: " + max.ToString("F2");
    _LatencyMinLabel.text = "Min: " + min.ToString("F2");

    // if they turned this on by the debug menu it's not a warning
    _WarningLabel.gameObject.SetActive(avg > kMaxThresholdBeforeWarning_ms);
  }

  private void HandleCloseButtonClick() {
    SaveEnabled(false);
    Destroy(gameObject);
  }
}
