using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FPSCalculator : MonoBehaviour {
  
  [SerializeField]
  private SimpleFPSCounterLabel fpsCounterPrefab_;
  private SimpleFPSCounterLabel fpsCounterInstance_ = null;

  /// <summary>
  /// Number of frames to sample for the avg fps numbers
  /// </summary>
  private const int AVG_FPS_SAMPLE_SIZE = 5;

  private const float SECONDS_PER_MINUTE = 60.0f;

  private int currentFPS_ = 0;
  private int numFramesInSecond_ = 0;
  private float secondTimeTracker_ = 0f;

  private List<int> fpsSamples_ = new List<int>();

  private PerformancePane performancePaneInstance_ = null;

	// Use this for initialization
	private void Start () {
	  // Set the target frame rate super high so that we get
    // accurate numbers on device. (IOS normally caps at 30)
    Application.targetFrameRate = 1000;
    fpsSamples_ = new List<int>();

    PerformancePane.PerformancePaneOpened += OnPerformancePaneOpened;
	}
	
	// Update is called once per frame
	private void Update () {
    float lastFrameSeconds = Time.deltaTime;
    secondTimeTracker_ += lastFrameSeconds;
    numFramesInSecond_++;

    if (secondTimeTracker_ >= 1f) {
      // Calculate the current fps
      currentFPS_ = numFramesInSecond_;

      // Add to samples
      fpsSamples_.Add(currentFPS_);
      
      // Truncate list of samples
      while (fpsSamples_.Count > SECONDS_PER_MINUTE){
        fpsSamples_.RemoveAt(0);
      }

      // Update UI
      float avgFPS = 0f;
      if (performancePaneInstance_ != null || fpsCounterInstance_ != null) {
        avgFPS = CalculateAvgFPS();
      }
      if (performancePaneInstance_ != null) {
        performancePaneInstance_.SetFPS(currentFPS_);
        performancePaneInstance_.SetAvgFPS(avgFPS);
        performancePaneInstance_.SetFPSPerMinute(CalculateAvgFramesPerMinute());
      }
      if(fpsCounterInstance_ != null) {
        fpsCounterInstance_.SetFPS(currentFPS_);
        fpsCounterInstance_.SetAvgFPS(avgFPS);
      }

      // Reset time tracker
      secondTimeTracker_ -= 1f;
      numFramesInSecond_ = 0;
    }
	}

  private void OnDestroy(){
    if (performancePaneInstance_ != null) {
      performancePaneInstance_.PerformancePaneClosed -= OnPerformancePaneClosed;
      performancePaneInstance_.PerformanceCounterButtonClicked -= OnPerformanceCounterButtonClicked;
    }
  }

  private void OnPerformancePaneOpened (PerformancePane performancePane)
  {
    performancePaneInstance_ = performancePane;
    performancePaneInstance_.PerformancePaneClosed += OnPerformancePaneClosed;
    performancePaneInstance_.PerformanceCounterButtonClicked += OnPerformanceCounterButtonClicked;
    performancePaneInstance_.SetFPS(currentFPS_);
    performancePaneInstance_.SetAvgFPS(CalculateAvgFPS());
    performancePaneInstance_.SetFPSPerMinute(CalculateAvgFramesPerMinute());
  }

  private void OnPerformancePaneClosed ()
  {
    performancePaneInstance_.PerformancePaneClosed -= OnPerformancePaneClosed;
    performancePaneInstance_ = null;
  }

  private void OnPerformanceCounterButtonClicked ()
  {
    // Create counter instance if it doesn't exist
    if (fpsCounterInstance_ == null) {
      GameObject fpsCounter = UIManager.CreateUI(fpsCounterPrefab_.gameObject);
      fpsCounterInstance_ = fpsCounter.GetComponent<SimpleFPSCounterLabel>();
      fpsCounterInstance_.SetFPS(CalculateAvgFPS());
    }
  }

  private float CalculateAvgFPS() {
    // Recalculate average fps according to sample size
    int startIndex = fpsSamples_.Count - AVG_FPS_SAMPLE_SIZE;
    if (startIndex < 0){
      startIndex = 0;
    }
    int framesInSample = 0;
    for (int i = startIndex; i < fpsSamples_.Count; i++){
      framesInSample += fpsSamples_[i];
    }
    int numSamples = Mathf.Min(AVG_FPS_SAMPLE_SIZE, fpsSamples_.Count);
    return ((float)framesInSample) / numSamples;
  }

  private float CalculateAvgFramesPerMinute() {
    // Recalculate average fps for minute
    int framesInMinute = 0;
    foreach(int numFrames in fpsSamples_){
      framesInMinute += numFrames;
    }
    return ((float)framesInMinute) / fpsSamples_.Count;
  }
}
