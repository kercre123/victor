using UnityEngine;
using Cozmo.UI;
using System;
using System.Collections;
using DataPersistence;

namespace Cozmo {
  public class PauseManager : MonoBehaviour {

#if ENABLE_DEBUG_PANEL
    public void FakeBatteryAlert() {
      if (!ListeningForBatteryLevel) {
        DAS.Warn("robot.low_battery", "blocked bad fake battery alert");
        return;
      }
      if (!sLowBatteryEventLogged) {
        sLowBatteryEventLogged = true;
        DAS.Event("robot.low_battery", "");
      }
      OpenLowBatteryDialog();
    }
#endif

    private static PauseManager _Instance;
    public Action OnPauseDialogOpen;
    private bool _IsPaused = false;
    private AlertView _GoToSleepDialog = null;
    private bool _IsOnChargerToSleep = false;
    private bool _StartedIdleTimeout = false;
    private bool _IdleTimeOutEnabled = true;
    private float _ShouldPlayWakeupTimestamp = -1;

    private const float _kMaxValidBatteryVoltage = 4.2f;
    private float _LowPassFilteredVoltage = _kMaxValidBatteryVoltage;
    private bool _LowBatteryAlertTriggered = false;
    private AlertView _LowBatteryDialog = null;
    public bool ListeningForBatteryLevel = false;
    public static bool sLowBatteryEventLogged = false;

    private AlertView _SleepCozmoConfirmDialog;

    public bool IsAnyDialogOpen {
      get {
        return (IsConfirmSleepDialogOpen || IsGoToSleepDialogOpen || IsLowBatteryDialogOpen);
      }
    }

    public bool IsConfirmSleepDialogOpen { get { return (null != _SleepCozmoConfirmDialog); } }
    public bool IsGoToSleepDialogOpen { get { return (null != _GoToSleepDialog); } }
    public bool IsLowBatteryDialogOpen { get { return (null != _LowBatteryDialog); } }
    public bool IsIdleTimeOutEnabled {
      get { return _IdleTimeOutEnabled; }
      set { _IdleTimeOutEnabled = value; }
    }

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
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnectionMessage);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.ReactionaryBehaviorTransition>(HandleReactionaryBehavior);
      DasTracker.Instance.TrackAppStartup();
    }

    private void Update() {
      if (!ListeningForBatteryLevel) {
        return;
      }
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      // Battery voltage gets initialized to the float maxvalue, so ignore it until it's valid 
      if (null != robot && robot.BatteryVoltage <= _kMaxValidBatteryVoltage) {
        _LowPassFilteredVoltage = _LowPassFilteredVoltage * Settings.FilterSmoothingWeight + (1.0f - Settings.FilterSmoothingWeight) * robot.BatteryVoltage;

        if (!_IsPaused && !IsConfirmSleepDialogOpen && !IsGoToSleepDialogOpen && _LowPassFilteredVoltage < Settings.LowBatteryVoltageValue && !_LowBatteryAlertTriggered) {
          if (!sLowBatteryEventLogged) {
            sLowBatteryEventLogged = true;
            DAS.Event("robot.low_battery", "");
          }
          OpenLowBatteryDialog();
        }
      }
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.GoingToSleep>(HandleGoingToSleep);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnectionMessage);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ReactionaryBehaviorTransition>(HandleReactionaryBehavior);
    }

    private void OnApplicationFocus(bool focusStatus) {
      DAS.Debug("PauseManager.OnApplicationFocus", "Application focus: " + focusStatus);
    }

    private void OnApplicationPause(bool bPause) {
      DAS.Debug("PauseManager.OnApplicationPause", "Application pause: " + bPause);

      if (bPause && DataPersistence.DataPersistenceManager.Instance != null) {
        // always save on pause
        DataPersistence.DataPersistenceManager.Instance.Save();
      }

      HandleApplicationPause(bPause);
    }

    private void OnApplicationQuit() {
      DasTracker.Instance.TrackAppQuit();
    }

    private void HandleReactionaryBehavior(Anki.Cozmo.ExternalInterface.ReactionaryBehaviorTransition message) {
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        return;
      }
      if ((IsGoToSleepDialogOpen && message.reactionaryBehaviorType != Anki.Cozmo.BehaviorType.ReactToOnCharger)
          || IsConfirmSleepDialogOpen) {
        StopIdleTimeout();
        _IsOnChargerToSleep = false;
        CloseGoToSleepDialog();
        CloseConfirmSleepDialog();
      }
    }

    private void HandleApplicationPause(bool shouldBePaused) {
      // When pausing, try to close any minigame that's open and reset robot state to Idle
      if (!_IsPaused && shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application being paused");
        _IsPaused = true;

        Cozmo.HomeHub.HomeHub hub = Cozmo.HomeHub.HomeHub.Instance;
        if (null != hub) {
          hub.CloseMiniGameImmediately();
        }

        _ShouldPlayWakeupTimestamp = -1;
        if (!_IsOnChargerToSleep && _IdleTimeOutEnabled) {
          StartIdleTimeout(Settings.AppBackground_TimeTilSleep_sec, Settings.AppBackground_TimeTilDisconnect_sec);
          // Set up a timer so that if we unpause after starting the goToSleep, we'll do the wakeup (using a little buffer past beginning of sleep)
          _ShouldPlayWakeupTimestamp = Time.realtimeSinceStartup + Settings.AppBackground_TimeTilSleep_sec + Settings.AppBackground_SleepAnimGetInBuffer_sec;
        }

        // Let the engine know that we're being paused
        RobotEngineManager.Instance.SendGameBeingPaused(true);

        DasTracker.Instance.TrackAppBackgrounded();

        RobotEngineManager.Instance.FlushChannelMessages();
      }
      // When unpausing, put the robot back into freeplay
      else if (_IsPaused && !shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application unpaused");
        _IsPaused = false;

        DasTracker.Instance.TrackAppResumed();

        // Let the engine know that we're being unpaused
        RobotEngineManager.Instance.SendGameBeingPaused(false);

        // If the go to sleep dialog is open, the user has selected sleep and there's no turning back, so don't wake up Cozmo
        if (!IsGoToSleepDialogOpen) {
          bool shouldPlayWakeup = false;
          if (!_IsOnChargerToSleep && _IdleTimeOutEnabled) {
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
    }

    private void HandleConfirmSleepCozmoOnChargerButtonTapped() {
      StartPlayerInducedSleep(true);
    }

    private void HandleConfirmSleepCozmoButtonTapped() {
      StartPlayerInducedSleep(false);
    }

    public void StartPlayerInducedSleep(bool fromCharger) {
      if (!fromCharger) {
        Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
        if (null != robot) {
          robot.EnableReactionaryBehaviors(false);
        }
      }
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
      else {
        // If this is fired because we are returning from backgrounding the app
        // but Cozmo has disconnected, CloseAllDialogs and handle it like a disconnect
        PauseManager.Instance.PauseManagerReset();
        IntroManager.Instance.ForceBoot();
      }
    }

    // Handles message sent from engine when the player puts cozmo on the charger.
    private void HandleGoingToSleep(Anki.Cozmo.ExternalInterface.GoingToSleep msg) {
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        return;
      }
      CloseLowBatteryDialog();
      CloseConfirmSleepDialog();
      _IsOnChargerToSleep = true;
      OpenConfirmSleepCozmoDialog(handleOnChargerSleepCancel: true);
    }

    private void HandleDisconnectionMessage(Anki.Cozmo.ExternalInterface.RobotDisconnected msg) {
      PauseManagerReset();
    }

    private void PauseManagerReset() {
      CloseAllDialogs();
      ListeningForBatteryLevel = false;
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

      Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
      if (null != robot) {
        robot.EnableReactionaryBehaviors(true);
      }
    }

    public void OpenConfirmSleepCozmoDialog(bool handleOnChargerSleepCancel) {
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        return;
      }
      if (IsGoToSleepDialogOpen) {
        // already going to sleep, so do nothing.
        return;
      }

      CloseLowBatteryDialog();
      if (!IsConfirmSleepDialogOpen) {
        Anki.Cozmo.Audio.AudioEventParameter openEvt = Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);

        _SleepCozmoConfirmDialog = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab,
                                                      preInitFunc: (AlertView alertView) => {
                                                        alertView.OpenAudioEvent = openEvt;
                                                      },
                                                      overrideBackgroundDim: null,
                                                      overrideCloseOnTouchOutside: false);
        _SleepCozmoConfirmDialog.SetCloseButtonEnabled(false);

        if (handleOnChargerSleepCancel) {
          _SleepCozmoConfirmDialog.SetSecondaryButton(LocalizationKeys.kButtonCancel, HandleOnChargerSleepCancel);
          _SleepCozmoConfirmDialog.SetPrimaryButton(LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalButtonConfirm,
                                                    HandleConfirmSleepCozmoOnChargerButtonTapped,
                                                    Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Disconnect));
        }
        else {
          _SleepCozmoConfirmDialog.SetSecondaryButton(LocalizationKeys.kButtonCancel);
          _SleepCozmoConfirmDialog.SetPrimaryButton(LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalButtonConfirm,
                                                    HandleConfirmSleepCozmoButtonTapped,
                                                    Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Disconnect));
        }

        _SleepCozmoConfirmDialog.TitleLocKey = LocalizationKeys.kSettingsSleepCozmoPanelConfirmationModalTitle;
        _SleepCozmoConfirmDialog.DescriptionLocKey = LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalDescription;
        if (OnPauseDialogOpen != null) {
          OnPauseDialogOpen.Invoke();
        }
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
        Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab,
                                                          preInitFunc: (AlertView view) => {
                                                            view.OpenAudioEvent = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;
                                                          },
                                                          overrideCloseOnTouchOutside: false);
        alertView.SetCloseButtonEnabled(false);
        alertView.TitleLocKey = LocalizationKeys.kConnectivityCozmoSleepTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kConnectivityCozmoSleepDesc;
        _GoToSleepDialog = alertView;
        // Set Music State
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Sleep);
        if (OnPauseDialogOpen != null) {
          OnPauseDialogOpen.Invoke();
        }
      }
    }

    private void OpenLowBatteryDialog() {
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        return;
      }
      if (!IsLowBatteryDialogOpen) {
        Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_LowBattery);
        alertView.SetCloseButtonEnabled(true);
        alertView.DimBackground = true;
        alertView.TitleLocKey = LocalizationKeys.kConnectivityCozmoLowBatteryTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kConnectivityCozmoLowBatteryDesc;
        _LowBatteryDialog = alertView;
        _LowBatteryAlertTriggered = true;

        if (OnPauseDialogOpen != null) {
          OnPauseDialogOpen.Invoke();
        }
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
