using UnityEngine;
using Cozmo.UI;
using System.Collections;

namespace Cozmo {
  public class PauseManager : MonoBehaviour {

    private static PauseManager _Instance;
    private bool _IsPaused = false;
    private AlertView _GoToSleepDialog = null;
    private bool _IsOnChargerToSleep = false;
    private bool _StartedIdleTimeout = false;
    private float _ShouldPlayWakeupTimestamp = -1;

    private const float _kMaxValidBatteryVoltage = 4.2f;
    private float _LowPassFilteredVoltage = _kMaxValidBatteryVoltage;
    private bool _LowBatteryAlertTriggered = false;
    private AlertView _LowBatteryDialog = null;
    public static bool _LowBatteryEventLogged = false;

    private AlertView _SleepCozmoConfirmDialog;

    public bool IsConfirmSleepDialogOpen { get { return (null != _SleepCozmoConfirmDialog); } }
    public bool IsGoToSleepDialogOpen { get { return (null != _GoToSleepDialog); } }
    public bool IsLowBatteryDialogOpen { get { return (null != _LowBatteryDialog); } }

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

    private Cozmo.Settings.DefaultSettingsValuesConfig Settings { get { return Cozmo.Settings.DefaultSettingsValuesConfig.Instance; } }

    private void Awake() {
      Instance = this;
    }

    private void Start() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.GoingToSleep>(HandleGoingToSleep);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnection);
    }

    private void Update() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      // Battery voltage gets initialized to the float maxvalue, so ignore it until it's valid 
      if (null != robot && robot.BatteryVoltage <= _kMaxValidBatteryVoltage) {
        _LowPassFilteredVoltage = _LowPassFilteredVoltage * Settings.FilterSmoothingWeight + (1.0f - Settings.FilterSmoothingWeight) * robot.BatteryVoltage;

        if (!_IsPaused && !IsConfirmSleepDialogOpen && !IsGoToSleepDialogOpen && _LowPassFilteredVoltage < Settings.LowBatteryVoltageValue && !_LowBatteryAlertTriggered) {
          if (!_LowBatteryEventLogged) {
            _LowBatteryEventLogged = true;
            DAS.Event("robot.low_battery", "");
          }
          OpenLowBatteryDialog();
        }
      }
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
        CloseLowBatteryDialog();
        CloseConfirmSleepDialog();

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        if (null != hub) {
          hub.CloseMiniGameImmediately();
        }

        _ShouldPlayWakeupTimestamp = -1;
        if (!_IsOnChargerToSleep) {
          StartIdleTimeout(Settings.AppBackground_TimeTilSleep_sec, Settings.AppBackground_TimeTilDisconnect_sec);
          // Set up a timer so that if we unpause after starting the goToSleep, we'll do the wakeup (using a little buffer past beginning of sleep)
          _ShouldPlayWakeupTimestamp = Time.realtimeSinceStartup + Settings.AppBackground_TimeTilSleep_sec + Settings.AppBackground_SleepAnimGetInBuffer_sec;
        }

        // Let the engine know that we're being paused
        RobotEngineManager.Instance.SendGameBeingPaused(true);

        RobotEngineManager.Instance.FlushChannelMessages();
      }
      // When unpausing, put the robot back into freeplay
      else if (_IsPaused && !shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application unpaused");
        _IsPaused = false;

        // Let the engine know that we're being unpaused
        RobotEngineManager.Instance.SendGameBeingPaused(false);

        bool shouldPlayWakeup = false;
        if (!_IsOnChargerToSleep) {
          StopIdleTimeout();
          if (_ShouldPlayWakeupTimestamp > 0 && Time.realtimeSinceStartup >= _ShouldPlayWakeupTimestamp) {
            shouldPlayWakeup = true;
          }
        }

        if (shouldPlayWakeup) {
          IRobot robot = RobotEngineManager.Instance.CurrentRobot;
          if (null != robot) {
            robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.GoToSleepGetOut, HandleFinishedWakeup);
          }
        }
        else {
          HandleFinishedWakeup(true);
        }
      }
    }

    private void HandleConfirmSleepCozmoButtonTapped() {
      StartIdleTimeout(Settings.PlayerSleepCozmo_TimeTilSleep_sec, Settings.PlayerSleepCozmo_TimeTilDisconnect_sec);
      OpenGoToSleepDialogAndFreezeUI();
    }

    private void HandleFinishedWakeup(bool success) {
      Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;

      if (null != robot && null != hub) {
        hub.StartFreeplay(robot);
        robot.EnableCubeSleep(false);
      }
    }

    // Handles message sent from engine when the player puts cozmo on the charger.
    private void HandleGoingToSleep(Anki.Cozmo.ExternalInterface.GoingToSleep msg) {
      CloseLowBatteryDialog();
      CloseConfirmSleepDialog();
      _IsOnChargerToSleep = true;
      OpenConfirmSleepCozmoDialog(handleOnChargerSleepCancel: true);
    }

    private void HandleDisconnection(Anki.Cozmo.ExternalInterface.RobotDisconnected msg) {
      CloseAllDialogs();
      _StartedIdleTimeout = false;
      _IsOnChargerToSleep = false;
      _LowPassFilteredVoltage = _kMaxValidBatteryVoltage;
      _LowBatteryAlertTriggered = false;
    }

    private void StartIdleTimeout(float sleep_sec, float disconnect_sec) {
      if (!_StartedIdleTimeout) {
        Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
        if (null != robot) {
          robot.ResetRobotState(null);
          robot.EnableCubeSleep(true);
        }

        RobotEngineManager.Instance.StartIdleTimeout(faceOffTime_s: sleep_sec, disconnectTime_s: disconnect_sec);
        _StartedIdleTimeout = true;
      }
    }

    private void StopIdleTimeout() {
      _StartedIdleTimeout = false;
      RobotEngineManager.Instance.CancelIdleTimeout();
    }

    public void OpenConfirmSleepCozmoDialog(bool handleOnChargerSleepCancel) {
      CloseLowBatteryDialog();
      if (!IsConfirmSleepDialogOpen) {
        _SleepCozmoConfirmDialog = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab, preInitFunc: null,
                                                    overrideBackgroundDim: null, overrideCloseOnTouchOutside: false);
        _SleepCozmoConfirmDialog.Initialize(false);
        _SleepCozmoConfirmDialog.SetCloseButtonEnabled(false);
        _SleepCozmoConfirmDialog.SetPrimaryButton(LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalButtonConfirm,
                                                  HandleConfirmSleepCozmoButtonTapped,
                                                  Anki.Cozmo.Audio.AudioEventParameter.SFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Cozmo_Disconnect));

        if (handleOnChargerSleepCancel) {
          _SleepCozmoConfirmDialog.SetSecondaryButton(LocalizationKeys.kButtonCancel, HandleOnChargerSleepCancel);
        }
        else {
          _SleepCozmoConfirmDialog.SetSecondaryButton(LocalizationKeys.kButtonCancel);
        }

        _SleepCozmoConfirmDialog.TitleLocKey = LocalizationKeys.kSettingsSleepCozmoPanelConfirmationModalTitle;
        _SleepCozmoConfirmDialog.DescriptionLocKey = LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalDescription;
      }
    }

    private void HandleOnChargerSleepCancel() {
      StopIdleTimeout();
      _IsOnChargerToSleep = false;
    }

    private void OpenGoToSleepDialogAndFreezeUI() {
      CloseLowBatteryDialog();
      CloseConfirmSleepDialog();
      if (!IsGoToSleepDialogOpen) {
        Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: false);
        alertView.SetCloseButtonEnabled(false);
        alertView.TitleLocKey = LocalizationKeys.kConnectivityCozmoSleepTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kConnectivityCozmoSleepDesc;
        _GoToSleepDialog = alertView;
      }
    }

    private void OpenLowBatteryDialog() {
      if (!IsLowBatteryDialogOpen) {
        Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab);
        alertView.SetCloseButtonEnabled(true);
        alertView.SetPrimaryButton(LocalizationKeys.kButtonClose, null);
        alertView.TitleLocKey = LocalizationKeys.kConnectivityCozmoLowBatteryTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kConnectivityCozmoLowBatteryDesc;
        _LowBatteryDialog = alertView;
        _LowBatteryAlertTriggered = true;
      }
    }

    private void CloseAllDialogs() {
      CloseConfirmSleepDialog();
      CloseGoToSleepDialog();
      CloseLowBatteryDialog();
    }

    private void CloseConfirmSleepDialog() {
      if (null != _SleepCozmoConfirmDialog) {
        _SleepCozmoConfirmDialog.CloseView();
        _SleepCozmoConfirmDialog = null;
      }
    }

    private void CloseGoToSleepDialog() {
      if (null != _GoToSleepDialog) {
        _GoToSleepDialog.CloseView();
        _GoToSleepDialog = null;
      }
    }

    private void CloseLowBatteryDialog() {
      if (null != _LowBatteryDialog) {
        _LowBatteryDialog.CloseView();
        _LowBatteryDialog = null;
      }
    }
  }
}
