using System.Collections.Generic;
using Anki.Debug;

namespace DataPersistence {
  // Sticky props that will be compiled out of final version go here.
  // Since some of the debug menu comes from engine and in most cases we don't want it to stick so it's easily resetable
  // things that go here should be the exceptions.
  [System.Serializable]
  public class DebugProfile {
    public bool SOSLoggerEnabled;
    public bool LatencyDisplayEnabled;
    public bool DebugPauseEnabled;
    public bool RunPressDemo;
    public Dictionary<string, List<FakeTouch>> FakeTouchRecordings;
    public bool NoFreeplayOnStart;
    public bool SkipFirmwareAutoUpdate;

    public DebugProfile() {
      SOSLoggerEnabled = false;
      LatencyDisplayEnabled = false;
      DebugPauseEnabled = false;
      RunPressDemo = false;
      FakeTouchRecordings = new Dictionary<string, List<FakeTouch>>();
      NoFreeplayOnStart = false;
      SkipFirmwareAutoUpdate = false;

      DebugConsoleData.Instance.AddConsoleVar("NoFreeplayOnStart", "Animator", this);
      DebugConsoleData.Instance.AddConsoleVar("SkipFirmwareAutoUpdate", "Firmware", this);
      DebugConsoleData.Instance.DebugConsoleVarUpdated += HandleDebugConsoleVarUpdated;
    }

    private void HandleDebugConsoleVarUpdated(string varName) {
      if (varName == "NoFreeplayOnStart") {
        DataPersistence.DataPersistenceManager.Instance.Save();
        if (RobotEngineManager.Instance != null && RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(!NoFreeplayOnStart);
        }
      }

      if (varName == "SkipFirmwareAutoUpdate") {
        DataPersistence.DataPersistenceManager.Instance.Save();
      }
    }
  }
}