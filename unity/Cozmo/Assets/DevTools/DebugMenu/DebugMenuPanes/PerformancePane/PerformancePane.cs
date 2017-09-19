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

  private void Start() {
    _ShowFPSCounterButton.onClick.AddListener(HandleShowCounterButtonClicked);
    InitQualityLevelDropdown();
    _QualityDropDown.onValueChanged.AddListener(HandleQualityChanged);
    InitPerfHUDDropdown();
    _PerfWarningHUDDropdown.onValueChanged.AddListener(HandlePerfHUDModeChanged);
    RaisePerformancePaneOpened(this);
  }

  private void OnDestroy() {
    _ShowFPSCounterButton.onClick.RemoveListener(HandleShowCounterButtonClicked);
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
    Cozmo.PerformanceManager.Instance.PerfHUD.SetPerfWarningDisplayState((PerfWarningDisplay.PerfWarningDisplayMode)value_changed);
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
