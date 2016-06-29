using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// Sticky props that will be compiled out of final version go here.
// Since some of the debug menu comes from engine and in most cases we don't want it to stick so it's easily resetable
// things that go here should be the exceptions.
[System.Serializable]
public class DebugProfile {
  public bool SOSLoggerEnabled;
  public bool LatencyDisplayEnabled;
  public bool DebugPauseEnabled;
  public bool RunPressDemo;

  public DebugProfile() {
    SOSLoggerEnabled = false;
    LatencyDisplayEnabled = false;
    DebugPauseEnabled = false;
    RunPressDemo = false;
  }
}
