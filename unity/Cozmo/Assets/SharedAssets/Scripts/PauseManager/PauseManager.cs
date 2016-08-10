using UnityEngine;
using System.Collections;

namespace Cozmo {
  public class PauseManager : MonoBehaviour {

    [Range(-1f,120f)]
    public float TimeTilFaceOff_s;

    [Range(-1f,120f)]
    public float TimeTilDisconnect_s;
    
    private static PauseManager _Instance;
    private bool _IsPaused = false;

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
      DAS.Debug("PauseManager.OnApplicationFocus", "Application focus: " + focusStatus);
    }

    private void OnApplicationPause(bool bPause) {
      DAS.Debug("PauseManager.OnApplicationPause", "Application pause: " + bPause);
      HandleApplicationPause(bPause);
    }

    private void HandleApplicationPause(bool shouldBePaused) {
      // When pausing, try to close any minigame that's open and reset robot state to Idle
      if (!_IsPaused && shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application being paused");
        _IsPaused = true;

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        if (null != hub) {
          hub.CloseMiniGameImmediately();

          Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
          if (null != robot) {
            robot.ResetRobotState(null);
          }
        }
        RobotEngineManager.Instance.StartIdleTimeout(faceOffTime_s: TimeTilFaceOff_s, disconnectTime_s: TimeTilDisconnect_s);
      }
      // When unpausing, put the robot back into freeplay
      else if (_IsPaused && !shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application unpaused");
        _IsPaused = false;

        RobotEngineManager.Instance.CancelIdleTimeout();

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;

        if (null != robot && null != hub) {
          hub.StartFreeplay(robot);
        }
      }
    }
  }
}
