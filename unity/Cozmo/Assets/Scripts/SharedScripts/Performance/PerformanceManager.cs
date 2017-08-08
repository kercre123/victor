using UnityEngine;
using System.Collections.Generic;
using System.Linq;

namespace Cozmo {
  public class PerformanceManager : MonoBehaviour {

    private static PerformanceManager _Instance;

    // These correspond to the 6 QualitySettings.
    [SerializeField]
    private float[] _LowThresholdPercents;

    // seconds history we're tracking
    [SerializeField]
    private int _AvgFPSSampleSize = 30;

    private LinkedList<int> _FpsSamples;
    private int _NumFramesInSecond = 0;
    private float _SecondTimeTracker = 0f;
    private bool _IsBackgrounded = false;
    private float _SecondsSinceQualityCheck = 0f;
    private bool _QualityForceSet = false;

    // don't use real quality settings since those cause artifacts and are mostly settings we don't use.
    // but the premade names are useful.
    private int _MaxQualityLevel;
    private int _QualityLevel;

    public static PerformanceManager Instance {
      get {
        if (_Instance == null) {
          string stackTrace = System.Environment.StackTrace;
          DAS.Error("PerformanceManager.NullInstance", "Do not access PerformanceManager until start");
          DAS.Debug("PerformanceManager.NullInstance.StackTrace", DASUtil.FormatStackTrace(stackTrace));
          HockeyApp.ReportStackTrace("PerformanceManager.NullInstance", stackTrace);
        }
        return _Instance;
      }
      private set {
        if (_Instance != null) {
          DAS.Error("PerformanceManager.DuplicateInstance", "PerformanceManager Instance already exists");
        }
        _Instance = value;
      }
    }

    private void Awake() {
      Instance = this;
      _MaxQualityLevel = QualitySettings.names.Length;
      _QualityLevel = _MaxQualityLevel - 1;
      _FpsSamples = new LinkedList<int>();
    }

    private void Update() {
      if (_IsBackgrounded) {
        return;
      }
      float lastFrameSeconds = Time.deltaTime;
      _SecondTimeTracker += lastFrameSeconds;
      _NumFramesInSecond++;

      if (_SecondTimeTracker >= 1f) {
        _SecondsSinceQualityCheck += _SecondTimeTracker;
        // Add to samples
        _FpsSamples.AddLast(_NumFramesInSecond);
        if (_FpsSamples.Count >= _AvgFPSSampleSize) {
          // Have enough samples to get an average and haven't checked in several seconds
          // downscale again.
          if (_SecondsSinceQualityCheck > _AvgFPSSampleSize && !_QualityForceSet) {
            CheckQualitySettingsUpdate();
            _SecondsSinceQualityCheck = 0f;
          }
          // This only runs once per second so not too thrashy
          _FpsSamples.RemoveFirst();
        }

        // Reset time tracker
        _SecondTimeTracker -= 1f;
        _NumFramesInSecond = 0;
      }
    }

    private void OnApplicationPause(bool bPause) {
      // Pause taking samples
      _IsBackgrounded = bPause;
    }

    // Not using real unity quality settings because they cause weird bugs on iOS and mostly use
    // settings that don't effect us.
    public int GetQualitySetting() {
      return _QualityLevel;
    }

    // Once debug quality has been set, let it stay there.
    public void ForceSetQuality(int quality) {
      _QualityLevel = quality;
      _QualityForceSet = true;
    }

    private void CheckQualitySettingsUpdate() {
      int currQualityLevel = GetQualitySetting();
      int wantsQualityLevel = currQualityLevel;
      int targetFrameRate = Application.targetFrameRate;
      double actualAvgFPS = CalculateAvgFPS();
      // if we're only running at less than 40% of our target, downscale to lowest
      for (int i = 0; i < _LowThresholdPercents.Length; ++i) {
        double thresholdMax = _LowThresholdPercents[i] * targetFrameRate;
        if (actualAvgFPS <= thresholdMax) {
          wantsQualityLevel = i;
          break;
        }
      }
      // Don't upscale usually, once we go down stay down
      if (wantsQualityLevel < currQualityLevel) {
        _QualityLevel = wantsQualityLevel;
        DAS.Event("PerformanceManager.QualityLevelChange", wantsQualityLevel.ToString());
      }
    }

    private double CalculateAvgFPS() {
      if (_FpsSamples.Count >= _AvgFPSSampleSize) {
        return _FpsSamples.Average();
      }
      return 0.0f;
    }

  }
}
