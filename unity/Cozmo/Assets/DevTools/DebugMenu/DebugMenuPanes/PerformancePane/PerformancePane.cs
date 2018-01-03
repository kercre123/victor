using Anki.Cozmo.ExternalInterface;
using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;

public class PerformancePane : MonoBehaviour {

  public delegate void PerformancePaneOpenHandler(PerformancePane performancePane);

  public static event PerformancePaneOpenHandler OnPerformancePaneOpened;

  private static void RaisePerformancePaneOpened(PerformancePane performancePane) {
    if (OnPerformancePaneOpened != null) {
      OnPerformancePaneOpened(performancePane);
    }
  }

  public delegate void PerformancePaneClosedHandler();

  public event PerformancePaneClosedHandler OnPerformancePaneClosed;

  private void RaisePerformancePaneClosed() {
    if (OnPerformancePaneClosed != null) {
      OnPerformancePaneClosed();
    }
  }

  public delegate void PerformanceCounterButtonClickedHandler();

  public event PerformanceCounterButtonClickedHandler OnPerformanceCounterButtonClicked;

  private void RaisePerformanceCounterButtonClicked() {
    if (OnPerformanceCounterButtonClicked != null) {
      OnPerformanceCounterButtonClicked();
    }
  }

  [SerializeField]
  private Text _FpsLabel;

  [SerializeField]
  private Text _AvgFPSLabel;

  [SerializeField]
  private Text _AvgFPSPerMinuteLabel;

  [SerializeField]
  private Button _ShowFPSCounterButton;

  [SerializeField]
  private Dropdown _PerfWarningHUDDropdown;

  [SerializeField]
  private Text _BatteryVoltage;

  [SerializeField]
  private Dropdown _QualityDropDown;

  [SerializeField]
  private Button _PerfMetricResetButton;

  [SerializeField]
  private Button _PerfMetricStartButton;

  [SerializeField]
  private Button _PerfMetricStopButton;

  [SerializeField]
  private Button _PerfMetricDumpButton;

  [SerializeField]
  private Button _PerfMetricDumpAllButton;

  [SerializeField]
  private Button _PerfMetricDumpFilesButton;

  [SerializeField]
  private Text _PerfMetricStatus;


  private void Start() {
    _ShowFPSCounterButton.onClick.AddListener(HandleShowCounterButtonClicked);
    _PerfMetricResetButton.onClick.AddListener(HandlePerfMetricResetButtonClicked);
    _PerfMetricStartButton.onClick.AddListener(HandlePerfMetricStartButtonClicked);
    _PerfMetricStopButton.onClick.AddListener(HandlePerfMetricStopButtonClicked);
    _PerfMetricDumpButton.onClick.AddListener(HandlePerfMetricDumpButtonClicked);
    _PerfMetricDumpAllButton.onClick.AddListener(HandlePerfMetricDumpAllButtonClicked);
    _PerfMetricDumpFilesButton.onClick.AddListener(HandlePerfMetricDumpFilesButtonClicked);
    RobotEngineManager.Instance.AddCallback<PerfMetricStatus>(HandlePerfMetricStatusFromEngine);
    InitQualityLevelDropdown();
    _QualityDropDown.onValueChanged.AddListener(HandleQualityChanged);
    InitPerfHUDDropdown();
    _PerfWarningHUDDropdown.onValueChanged.AddListener(HandlePerfHUDModeChanged);
    RaisePerformancePaneOpened(this);

    RobotEngineManager.Instance.SendPerfMetricGetStatus();
  }

  private void OnDestroy() {
    _ShowFPSCounterButton.onClick.RemoveListener(HandleShowCounterButtonClicked);
    _PerfMetricResetButton.onClick.RemoveListener(HandlePerfMetricResetButtonClicked);
    _PerfMetricStartButton.onClick.RemoveListener(HandlePerfMetricStartButtonClicked);
    _PerfMetricStopButton.onClick.RemoveListener(HandlePerfMetricStopButtonClicked);
    _PerfMetricDumpButton.onClick.RemoveListener(HandlePerfMetricDumpButtonClicked);
    _PerfMetricDumpAllButton.onClick.RemoveListener(HandlePerfMetricDumpAllButtonClicked);
    _PerfMetricDumpFilesButton.onClick.RemoveListener(HandlePerfMetricDumpFilesButtonClicked);
    RobotEngineManager.Instance.RemoveCallback<PerfMetricStatus>(HandlePerfMetricStatusFromEngine);
    _QualityDropDown.onValueChanged.RemoveListener(HandleQualityChanged);
    RaisePerformancePaneClosed();
  }

  public void SetFPS(float newFPS) {
    _FpsLabel.text = newFPS.ToString();
  }

  public void SetAvgFPS(float newAvgFPS) {
    // Show to 2 decimal places
    _AvgFPSLabel.text = newAvgFPS.ToString("F2");
  }

  public void SetFPSPerMinute(float newFPSPerMinute) {
    _AvgFPSPerMinuteLabel.text = newFPSPerMinute.ToString("F2");
  }

  private void HandleShowCounterButtonClicked() {
    RaisePerformanceCounterButtonClicked();
  }

  private void HandlePerfMetricResetButtonClicked() {
    RobotEngineManager.Instance.SendPerfMetricCommand(PerfMetricCommandType.Reset);
  }

  private void HandlePerfMetricStartButtonClicked() {
    RobotEngineManager.Instance.SendPerfMetricCommand(PerfMetricCommandType.Start);
  }

  private void HandlePerfMetricStopButtonClicked() {
    RobotEngineManager.Instance.SendPerfMetricCommand(PerfMetricCommandType.Stop);
  }

  private void HandlePerfMetricDumpButtonClicked() {
    RobotEngineManager.Instance.SendPerfMetricCommand(PerfMetricCommandType.Dump);
  }

  private void HandlePerfMetricDumpAllButtonClicked() {
    RobotEngineManager.Instance.SendPerfMetricCommand(PerfMetricCommandType.DumpAll);
  }

  private void HandlePerfMetricDumpFilesButtonClicked() {
    RobotEngineManager.Instance.SendPerfMetricCommand(PerfMetricCommandType.DumpFiles);
  }

  private void HandlePerfMetricStatusFromEngine(PerfMetricStatus status) {
    _PerfMetricStatus.text = status.statusString;
    _PerfMetricStartButton.enabled = !status.isRecording;
    _PerfMetricStopButton.enabled = status.isRecording;
  }


  private void InitPerfHUDDropdown() {
    _PerfWarningHUDDropdown.ClearOptions();
    var enum_values = System.Enum.GetValues(typeof(PerfWarningDisplay.PerfWarningDisplayMode));
    List<UnityEngine.UI.Dropdown.OptionData> options = new List<UnityEngine.UI.Dropdown.OptionData>();
    for (int i = 0; i < enum_values.Length; ++i) {
      UnityEngine.UI.Dropdown.OptionData option = new UnityEngine.UI.Dropdown.OptionData();
      option.text = System.Enum.GetValues(typeof(PerfWarningDisplay.PerfWarningDisplayMode)).GetValue(i).ToString();
      options.Add(option);
    }
    _PerfWarningHUDDropdown.AddOptions(options);
    _PerfWarningHUDDropdown.value = (int)DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.PerfInfoDisplayMode;
    _PerfWarningHUDDropdown.Select();
    _PerfWarningHUDDropdown.RefreshShownValue();
  }

  private void HandlePerfHUDModeChanged(int value_changed) {
#if ANKI_DEV_CHEATS
    Cozmo.PerformanceManager.Instance.PerfHUD.SetPerfWarningDisplayState((PerfWarningDisplay.PerfWarningDisplayMode)value_changed);
#endif
  }

  private void InitQualityLevelDropdown() {
    string[] qualityNames = QualitySettings.names;
    _QualityDropDown.options.Clear();
    for (int i = 0; i < qualityNames.Length; ++i) {
      _QualityDropDown.options.Add(new Dropdown.OptionData(i + " " + qualityNames[i]));
    }
    _QualityDropDown.value = Cozmo.PerformanceManager.Instance.GetQualitySetting();
    _QualityDropDown.Select();
    _QualityDropDown.RefreshShownValue();

  }
  private void HandleQualityChanged(int value_changed) {
    Cozmo.PerformanceManager.Instance.ForceSetQuality(value_changed);
  }


  private void Update() {
    if (_BatteryVoltage != null) {
      string batteryVoltageStr = "Battery Voltage Not Available";
      RobotEngineManager rem = RobotEngineManager.Instance;
      if (rem != null) {
        IRobot robot = rem.CurrentRobot;
        if (robot != null) {
          batteryVoltageStr = robot.BatteryVoltage.ToString();
        }
      }
      _BatteryVoltage.text = batteryVoltageStr;
    }
  }
}
