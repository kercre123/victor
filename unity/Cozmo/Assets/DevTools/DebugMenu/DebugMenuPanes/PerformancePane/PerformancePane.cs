using UnityEngine;
using UnityEngine.UI;
using System.Collections;

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
  private Text _BatteryVoltage;

  [SerializeField]
  private Text _DeviceID;

  [SerializeField]
  private Text _AppRunID;

  [SerializeField]
  private Text _LastAppRunID;

  [SerializeField]
  private Text _ActiveVariantText;

  private void Start() {
    _ActiveVariantText.text = Screen.currentResolution + "\n" + Anki.Assets.AssetBundleManager.Instance.ActiveVariantsToString();
    _ShowFPSCounterButton.onClick.AddListener(HandleShowCounterButtonClicked);
    RaisePerformancePaneOpened(this);
    RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.ExternalInterface.DeviceDataMessage), HandleDeviceDataMessage);
    RobotEngineManager.Instance.SendRequestDeviceData();
  }

  private void OnDestroy() {
    _ShowFPSCounterButton.onClick.RemoveListener(HandleShowCounterButtonClicked);
    RaisePerformancePaneClosed();
    RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.ExternalInterface.DeviceDataMessage), HandleDeviceDataMessage);
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

  private void HandleDeviceDataMessage(object messageObject) {
    Anki.Cozmo.ExternalInterface.DeviceDataMessage message = (Anki.Cozmo.ExternalInterface.DeviceDataMessage)messageObject;
    for (int i = 0; i < message.dataList.Length; ++i) {
      Anki.Cozmo.DeviceDataPair currentPair = message.dataList[i];
      switch (currentPair.dataType) {
      case Anki.Cozmo.DeviceDataType.DeviceID: {
          _DeviceID.text = currentPair.dataValue;
          break;
        }
      case Anki.Cozmo.DeviceDataType.AppRunID: {
          _AppRunID.text = currentPair.dataValue;
          break;
        }
      case Anki.Cozmo.DeviceDataType.LastAppRunID: {
          _LastAppRunID.text = currentPair.dataValue;
          break;
        }
      default: {
          DAS.Debug("PerformancePane.HandleDeviceDataMessage.UnhandledDataType", currentPair.dataType.ToString());
          break;
        }
      }
    }
  }

  private void Update() {
    _BatteryVoltage.text = RobotEngineManager.Instance.CurrentRobot.BatteryVoltage.ToString();
  }
}
