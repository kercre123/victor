using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FPSCalculator : MonoBehaviour {
  
  [SerializeField]
  private SimpleFPSCounterLabel _FpsCounterPrefab;
  private SimpleFPSCounterLabel _FpsCounterInstance = null;

  /// <summary>
  /// Number of frames to sample for the avg fps numbers
  /// </summary>
  private const int AVG_FPS_SAMPLE_SIZE = 5;

  private const float SECONDS_PER_MINUTE = 60.0f;

  private int _CurrentFPS = 0;
  private int _NumFramesInSecond = 0;
  private float _SecondTimeTracker = 0f;

  private List<int> _FpsSamples = new List<int>();

  private PerformancePane _PerformancePaneInstance = null;

  // Use this for initialization
  private void Start() {
    // Set the target frame rate super high so that we get
    // accurate numbers on device. (IOS normally caps at 30)
    Application.targetFrameRate = 1000;
    _FpsSamples = new List<int>();

    PerformancePane.OnPerformancePaneOpened += HandlePerformancePaneOpened;
  }
	
  // Update is called once per frame
  private void Update() {
    float lastFrameSeconds = Time.deltaTime;
    _SecondTimeTracker += lastFrameSeconds;
    _NumFramesInSecond++;

    if (_SecondTimeTracker >= 1f) {
      // Calculate the current fps
      _CurrentFPS = _NumFramesInSecond;

      // Add to samples
      _FpsSamples.Add(_CurrentFPS);
      
      // Truncate list of samples
      while (_FpsSamples.Count > SECONDS_PER_MINUTE) {
        _FpsSamples.RemoveAt(0);
      }

      // Update UI
      float avgFPS = 0f;
      if (_PerformancePaneInstance != null || _FpsCounterInstance != null) {
        avgFPS = CalculateAvgFPS();
      }
      if (_PerformancePaneInstance != null) {
        _PerformancePaneInstance.SetFPS(_CurrentFPS);
        _PerformancePaneInstance.SetAvgFPS(avgFPS);
        _PerformancePaneInstance.SetFPSPerMinute(CalculateAvgFramesPerMinute());
      }
      if (_FpsCounterInstance != null) {
        _FpsCounterInstance.SetFPS(_CurrentFPS);
        _FpsCounterInstance.SetAvgFPS(avgFPS);
      }

      // Reset time tracker
      _SecondTimeTracker -= 1f;
      _NumFramesInSecond = 0;
    }
  }

  private void OnDestroy() {
    if (_PerformancePaneInstance != null) {
      _PerformancePaneInstance.OnPerformancePaneClosed -= HandlePerformancePaneClosed;
      _PerformancePaneInstance.OnPerformanceCounterButtonClicked -= HandlePerformanceCounterButtonClicked;
    }
  }

  private void HandlePerformancePaneOpened(PerformancePane performancePane) {
    _PerformancePaneInstance = performancePane;
    _PerformancePaneInstance.OnPerformancePaneClosed += HandlePerformancePaneClosed;
    _PerformancePaneInstance.OnPerformanceCounterButtonClicked += HandlePerformanceCounterButtonClicked;
    _PerformancePaneInstance.SetFPS(_CurrentFPS);
    _PerformancePaneInstance.SetAvgFPS(CalculateAvgFPS());
    _PerformancePaneInstance.SetFPSPerMinute(CalculateAvgFramesPerMinute());
  }

  private void HandlePerformancePaneClosed() {
    _PerformancePaneInstance.OnPerformancePaneClosed -= HandlePerformancePaneClosed;
    _PerformancePaneInstance = null;
  }

  private void HandlePerformanceCounterButtonClicked() {
    // Create counter instance if it doesn't exist
    if (_FpsCounterInstance == null) {
      GameObject fpsCounter = UIManager.CreateUI(_FpsCounterPrefab.gameObject);
      _FpsCounterInstance = fpsCounter.GetComponent<SimpleFPSCounterLabel>();
      _FpsCounterInstance.SetFPS(CalculateAvgFPS());
    }
  }

  private float CalculateAvgFPS() {
    // Recalculate average fps according to sample size
    int startIndex = _FpsSamples.Count - AVG_FPS_SAMPLE_SIZE;
    if (startIndex < 0) {
      startIndex = 0;
    }
    int framesInSample = 0;
    for (int i = startIndex; i < _FpsSamples.Count; i++) {
      framesInSample += _FpsSamples[i];
    }
    int numSamples = Mathf.Min(AVG_FPS_SAMPLE_SIZE, _FpsSamples.Count);
    return ((float)framesInSample) / numSamples;
  }

  private float CalculateAvgFramesPerMinute() {
    // Recalculate average fps for minute
    int framesInMinute = 0;
    foreach (int numFrames in _FpsSamples) {
      framesInMinute += numFrames;
    }
    return ((float)framesInMinute) / _FpsSamples.Count;
  }
}
