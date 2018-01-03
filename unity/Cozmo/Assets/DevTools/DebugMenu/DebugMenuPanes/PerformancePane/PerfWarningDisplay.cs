using UnityEngine;
using Anki.Cozmo;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.Debug;

public class PerfWarningDisplay : MonoBehaviour {
  [SerializeField]
  private PerfWarningSection _SectionPrefab;
  [SerializeField]
  private Transform _SectionContainer;

  private Dictionary<string, PerfWarningSection> _Sections;

  [SerializeField]
  private Button _CloseButton;

  private const string _kLatencySectionName = "Latency";
  private const string _kUnitySectionName = "Unity";
  private const string _kEngineFreqSectionName = "EngineFreq";
  private const string _kEngineSectionName = "Engine";

  // if the last 30 samples average at higher than these numbers, the section is displayed.
  private const float _kMaxWifiThresholdBeforeWarning_ms = 60.0f;
  private const float _kMaxUnityThresholdBeforeWarning_ms = 50.0f;  // 20 FPS
  private const float _kMaxEngineThresholdBeforeWarning_ms = 83.0f; // 12 FPS 
  private const float _kMaxEngineFreqThresholdBeforeWarning_ms = 83.0f;// 12 FPS

  public enum PerfWarningDisplayMode {
    TurnsOnWhenWarning,
    AlwaysOn,
    AlwaysOff,
  }
  private PerfWarningDisplayMode _Mode = PerfWarningDisplayMode.TurnsOnWhenWarning;

  // Use this for initialization
  private void Start() {
    _CloseButton.onClick.AddListener(HandleCloseButtonClick);

    _Sections = new Dictionary<string, PerfWarningSection>();
    _Sections.Add(_kLatencySectionName, CreateSection(_kLatencySectionName, _kMaxWifiThresholdBeforeWarning_ms));
    _Sections.Add(_kUnitySectionName, CreateSection(_kUnitySectionName, _kMaxUnityThresholdBeforeWarning_ms));
    _Sections.Add(_kEngineSectionName, CreateSection(_kEngineSectionName, _kMaxEngineThresholdBeforeWarning_ms));
    _Sections.Add(_kEngineFreqSectionName, CreateSection(_kEngineFreqSectionName, _kMaxEngineFreqThresholdBeforeWarning_ms));

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.LatencyMessage>(HandleDebugLatencyMsg);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DebugPerformanceTick>(HandleDebugPerfTickMsg);

    SetPerfWarningDisplayState(DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.PerfInfoDisplayMode, false);
  }

  public void SetPerfWarningDisplayState(PerfWarningDisplayMode mode, bool forceSave = true) {
    _Mode = mode;

    if (forceSave) {
      DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.PerfInfoDisplayMode = mode;
      DataPersistence.DataPersistenceManager.Instance.Save();
    }

    // TurnsOnWhenWarning will evaluate in the update loop
    bool forceAllActive = (_Mode == PerfWarningDisplayMode.AlwaysOn);
    _SectionContainer.parent.gameObject.SetActive(forceAllActive);
    foreach (var section in _Sections) {
      section.Value.gameObject.SetActive(forceAllActive);
    }
  }

  private string GetSectionData(string sectionName) {
    var section = _Sections[sectionName];
    return section.Average.ToString("F2") + "ms";
  }

  public string GetLatencySectionData() {
    return GetSectionData(_kLatencySectionName);
  }

  public string GetUnitySectionData() {
    return GetSectionData(_kUnitySectionName);
  }

  public string GetEngineFreqSectionData() {
    return GetSectionData(_kEngineFreqSectionName);
  }

  public string GetEngineSectionData() {
    return GetSectionData(_kEngineSectionName);
  }

  private PerfWarningSection CreateSection(string sectionName, float warnAboveThreshold) {
    PerfWarningSection section = GameObject.Instantiate(_SectionPrefab, _SectionContainer).GetComponent<PerfWarningSection>();
    bool sendDasOnlyWhenCrossing = (sectionName != _kLatencySectionName);
    section.Init(sectionName, warnAboveThreshold, sendDasOnlyWhenCrossing);
    return section;
  }

  private void OnDestroy() {
    _CloseButton.onClick.RemoveListener(HandleCloseButtonClick);
  }

  private void Update() {
    if (_Sections.Count == 0) {
      return;
    }
    float deltaTime_ms = Time.deltaTime * 1000; // seconds -> ms
    UpdateUI(_Sections[_kUnitySectionName], deltaTime_ms);

    // loop through all sections... 
    if (_Mode == PerfWarningDisplayMode.TurnsOnWhenWarning) {
      bool anyWarning = false;
      // any sections above threshold?
      // display
      foreach (var section in _Sections) {
        if (section.Value.IsWarning()) {
          anyWarning = true;
          section.Value.gameObject.SetActive(true);
        }
      }

      // make sure parent displayed.
      if (anyWarning) {
        _SectionContainer.parent.gameObject.SetActive(true);
      }
    }
  }

  private void HandleDebugPerfTickMsg(Anki.Cozmo.ExternalInterface.DebugPerformanceTick msg) {
    if (msg.systemName == _kEngineSectionName) {
      UpdateUI(_Sections[_kEngineSectionName], msg.lastTickTime);
    }
    else if (msg.systemName == _kEngineFreqSectionName) {
      UpdateUI(_Sections[_kEngineFreqSectionName], msg.lastTickTime);
    }
  }

  private void HandleDebugLatencyMsg(Anki.Cozmo.ExternalInterface.LatencyMessage msg) {
    PerfWarningSection latencySection = _Sections[_kLatencySectionName];
    // Latency is special since we don't need to track average ourselves and thats done in engine.
    latencySection.SetLargerLabel("Avg: " + msg.wifiLatency.avgTime_ms.ToString("F2") + "ms");
    latencySection.SetSmallLabel("Img: " + msg.imageLatency.avgTime_ms.ToString("F2") + "ms");
    latencySection.Average = msg.wifiLatency.avgTime_ms;
    latencySection.Update();
  }

  private void UpdateUI(PerfWarningSection section, float last_ms) {
    section.AddTimeSlice(last_ms);
    float average = section.Average;
    section.SetLargerLabel("Avg: " + average.ToString("F2") + "ms");
    section.SetSmallLabel("Last: " + last_ms.ToString("F2") + "ms");
    section.Update();
  }


  private void HandleCloseButtonClick() {
    // the next update might turn us back on if still in a warning state
    // people need to turn off in debug menu
    foreach (var section in _Sections) {
      section.Value.gameObject.SetActive(false);
    }
    _SectionContainer.parent.gameObject.SetActive(false);
  }
}
