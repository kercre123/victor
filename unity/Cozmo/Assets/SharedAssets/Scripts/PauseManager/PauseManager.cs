using UnityEngine;
using System.Collections;

namespace Cozmo {
  public class PauseManager : MonoBehaviour {
    
    private static PauseManager _Instance;

    public static PauseManager Instance {
      get {
        if (_Instance == null) {
          DAS.Error("PauseManager.NullInstance", "Do not access PauseManager until start: " + System.Environment.StackTrace);
        }
        return _Instance;
      }
      private set {
        if (_Instance != null) {
          DAS.Error("PauseManager.DuplicateInstance", "PauseManager Instance already exists");
        }
        _Instance = value;
      }
    }

    void Awake() {
      Instance = this;
    }

    private void OnApplicationFocus(bool focusStatus) {
      DAS.Debug(this, "App Focused: " + focusStatus);
    }

    private bool _IsPaused = false;
    private void OnApplicationPause(bool bPause) {

      // When pausing, try to close any minigame that's open and reset robot state to Idle
      if (!_IsPaused && bPause) {
        DAS.Debug("PauseManager.OnApplicationPause", "App put into background");
        _IsPaused = true;

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        if (null != hub) {
          hub.CloseMiniGameImmediately();

          Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
          if (null != robot) {
            robot.ResetRobotState(null);
          }
        }
      }
      // When unpausing, put the robot back into freeplay
      else if (_IsPaused && !bPause) {
        DAS.Debug("PauseManager.OnApplicationPause", "App put into foreground");
        _IsPaused = false;

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;

        if (null != robot && null != hub) {
          hub.StartFreeplay(robot);
        }
      }
    }
  }
}
