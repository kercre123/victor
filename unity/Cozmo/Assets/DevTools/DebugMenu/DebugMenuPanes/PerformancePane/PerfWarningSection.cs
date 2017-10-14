using UnityEngine;
using Anki.Cozmo;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.Debug;
using System.Linq;

public class PerfWarningSection : MonoBehaviour {
  // Set in the prefab
  [SerializeField]
  private CozmoText _Header;
  [SerializeField]
  private CozmoText _AvgLabel;
  [SerializeField]
  private CozmoText _LastLabel;

  // calculating... storing last...
  private float[] _TimeSlices;
  private int _MaxTimeSlices = 30;

  // 41 means we're below 24 FPS
  private float _WarnAboveTheshold_ms = 41.0f;
  private int _CurrIndex = 0;

  private float? _OverrideAverage;

  public float Average {
    get {
      return _OverrideAverage.HasValue ? _OverrideAverage.Value : _TimeSlices.Average();
    }
    set {
      _OverrideAverage = value;
    }
  }

  private bool _WasWarning;
  private string _SectionName;

  public void Init(string headerText, float warnAboveThreshold) {
    _TimeSlices = new float[_MaxTimeSlices];
    _SectionName = headerText;
    _Header.text = headerText;
    _AvgLabel.text = string.Empty;
    _LastLabel.text = string.Empty;
    _WarnAboveTheshold_ms = warnAboveThreshold;
    gameObject.SetActive(false);
    _WasWarning = false;
  }

  public void AddTimeSlice(float currTime) {
    // circular add.
    _TimeSlices[_CurrIndex] = currTime;
    _CurrIndex = (_CurrIndex + 1) % _MaxTimeSlices;
  }

  public bool IsWarning() {
    return Average > _WarnAboveTheshold_ms;
  }

  public void Update() {
    // if we've ever been in a bad state, then show red
    bool isWarning = IsWarning();
    if (isWarning != _WasWarning) {
      DAS.Info("PerfWarning.AboveThreshold." + _SectionName, Average.ToString());
    }
    if (isWarning && !_WasWarning) {
      _AvgLabel.color = Color.red;
      _Header.color = Color.red;
    }
    else if (!isWarning && _WasWarning) {
      _AvgLabel.color = Color.white;
    }

    _WasWarning = isWarning;
  }

  public void SetLargerLabel(string txt) {
    _AvgLabel.text = txt;
  }

  public void SetSmallLabel(string txt) {
    _LastLabel.text = txt;
  }
}
