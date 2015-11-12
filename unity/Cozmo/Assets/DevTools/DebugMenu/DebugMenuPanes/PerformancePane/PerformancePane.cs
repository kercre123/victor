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

  private void Start() {
    _ShowFPSCounterButton.onClick.AddListener(HandleShowCounterButtonClicked);
    RaisePerformancePaneOpened(this);
  }

  private void OnDestroy() {
    _ShowFPSCounterButton.onClick.RemoveListener(HandleShowCounterButtonClicked);
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
}
