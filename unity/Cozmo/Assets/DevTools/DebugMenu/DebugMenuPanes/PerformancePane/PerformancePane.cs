using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class PerformancePane : MonoBehaviour {

  public delegate void PerformancePaneOpenHandler(PerformancePane performancePane);
  public static event PerformancePaneOpenHandler PerformancePaneOpened;
  private static void RaisePerformancePaneOpened(PerformancePane performancePane){
    if (PerformancePaneOpened != null) {
      PerformancePaneOpened(performancePane);
    }
  }
  
  public delegate void PerformancePaneClosedHandler();
  public event PerformancePaneClosedHandler PerformancePaneClosed;
  private void RaisePerformancePaneClosed() {
    if (PerformancePaneClosed != null) {
      PerformancePaneClosed();
    }
  }
  
  public delegate void PerformanceCounterButtonClickedHandler();
  public event PerformanceCounterButtonClickedHandler PerformanceCounterButtonClicked;
  private void RaisePerformanceCounterButtonClicked() {
    if (PerformanceCounterButtonClicked != null) {
      PerformanceCounterButtonClicked();
    }
  }

  [SerializeField]
  private Text fpsLabel_;
  
  [SerializeField]
  private Text avgFPSLabel_;

  [SerializeField]
  private Text avgFPSPerMinuteLabel_;

  [SerializeField]
  private Button showFPSCounterButton_;

  private void Start () {
    showFPSCounterButton_.onClick.AddListener(OnShowCounterButtonClicked);
    RaisePerformancePaneOpened(this);
  }
  
  private void OnDestroy() {
    showFPSCounterButton_.onClick.RemoveListener(OnShowCounterButtonClicked);
    RaisePerformancePaneClosed();
  }

  public void SetFPS(float newFPS){
    fpsLabel_.text = newFPS.ToString();
  }
  
  public void SetAvgFPS(float newAvgFPS){
    // Show to 2 decimal places
    avgFPSLabel_.text = newAvgFPS.ToString("F2");
  }

  public void SetFPSPerMinute(float newFPSPerMinute){
    avgFPSPerMinuteLabel_.text = newFPSPerMinute.ToString("F2");
  }
  
  private void OnShowCounterButtonClicked() {
    RaisePerformanceCounterButtonClicked();
  }
}
