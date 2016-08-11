using UnityEngine;
using Cozmo.UI;
using System.Collections;

namespace Cozmo {
  public class PauseManager : MonoBehaviour {

    [Range(-1f,120f)]
    public float TimeTilFaceOff_s;

    [Range(-1f,120f)]
    public float TimeTilDisconnect_s;
    
    private static PauseManager _Instance;
    private bool _IsPaused = false;
    private AlertView _RequestDialog = null;
    private bool _IsOnChargerToSleep = false;

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

    private void Awake() {
      Instance = this;
    }

    private void Start() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.GoingToSleep>(HandleGoingToSleep);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnection);
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.GoingToSleep>(HandleGoingToSleep);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnection);
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

        if (!_IsOnChargerToSleep) {
          RobotEngineManager.Instance.StartIdleTimeout(faceOffTime_s: TimeTilFaceOff_s, disconnectTime_s: TimeTilDisconnect_s);
        }
      }
      // When unpausing, put the robot back into freeplay
      else if (_IsPaused && !shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application unpaused");
        _IsPaused = false;

        if (!_IsOnChargerToSleep) {
          RobotEngineManager.Instance.CancelIdleTimeout();
        }

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;

        if (null != robot && null != hub) {
          hub.StartFreeplay(robot);
        }
      }
    }

    private void HandleGoingToSleep(Anki.Cozmo.ExternalInterface.GoingToSleep msg) {
      CloseSleepDialog();
      _IsOnChargerToSleep = true;
      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside:false);
      alertView.TitleLocKey = LocalizationKeys.kConnectivityCozmoSleepTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kConnectivityCozmoSleepDesc;
      _RequestDialog = alertView;
    }

    private void HandleDisconnection(Anki.Cozmo.ExternalInterface.RobotDisconnected msg) {
      CloseSleepDialog();
      _IsOnChargerToSleep = false;
    }

    private void CloseSleepDialog() {
      if (_RequestDialog != null) {
        _RequestDialog.CloseView();
        _RequestDialog = null;
      }
    }
  }
}
