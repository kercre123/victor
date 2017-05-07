using UnityEngine;
using Cozmo.UI;
using System;
using DataPersistence;
using Cozmo.ConnectionFlow;

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
      OpenLowBatteryAlert();
    }
#endif

    private static PauseManager _Instance;
    public Action OnPauseDialogOpen;
    private bool _IsPaused = false;
    private AlertModal _GoToSleepDialog = null;
    private bool _IsOnChargerToSleep = false;
    private bool _StartedIdleTimeout = false;
    private bool _IdleTimeOutEnabled = true;
    private float _ShouldPlayWakeupTimestamp = -1;

    private const float _kMaxValidBatteryVoltage = 4.2f;
    private float _LowPassFilteredVoltage = _kMaxValidBatteryVoltage;
    private bool _LowBatteryAlertTriggered = false;
    private AlertModal _LowBatteryAlertInstance = null;
    public bool ListeningForBatteryLevel = false;
    public static bool sLowBatteryEventLogged = false;

    private AlertModal _SleepCozmoConfirmDialog;

    public bool IsConfirmSleepDialogOpen { get { return (null != _SleepCozmoConfirmDialog); } }
    public bool IsGoToSleepDialogOpen { get { return (null != _GoToSleepDialog); } }
    public bool IsLowBatteryDialogOpen { get { return (null != _LowBatteryAlertInstance); } }
    public bool IsIdleTimeOutEnabled {
      get { return _IdleTimeOutEnabled; }
      set { _IdleTimeOutEnabled = value; }
    }

    [SerializeField]
    private AlertModal _LowBatteryAlertPrefab;

    // Does singleton instance exist?
    public static bool InstanceExists {
      get {
        return (_Instance != null);
      }
    }

    public static PauseManager Instance {
      get {
        if (_Instance == null) {
          string stackTrace = System.Environment.StackTrace;
          DAS.Error("PauseManager.NullInstance", "Do not access PauseManager until start");
          DAS.Debug("PauseManager.NullInstance.StackTrace", DASUtil.FormatStackTrace(stackTrace));
          HockeyApp.ReportStackTrace("PauseManager.NullInstance", stackTrace);
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
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.ReactionTriggerTransition>(HandleReactionaryBehavior);
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
          OpenLowBatteryAlert();
        }
      }
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.GoingToSleep>(HandleGoingToSleep);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleDisconnectionMessage);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ReactionTriggerTransition>(HandleReactionaryBehavior);
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

    private void HandleReactionaryBehavior(Anki.Cozmo.ExternalInterface.ReactionTriggerTransition message) {
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        return;
      }
      if (IsConfirmSleepDialogOpen && message.newTrigger != Anki.Cozmo.ReactionTrigger.PlacedOnCharger) {
        StopIdleTimeout();
        _IsOnChargerToSleep = false;
        CloseGoToSleepDialog();
        CloseConfirmSleepDialog();

        DAS.Debug("PauseManager.HandleReactionaryBehavior.StopSleep", "Transition to: " + message.newTrigger.ToString() + " Transitioning From: " + message.oldTrigger.ToString());
      }
    }

    private void HandleApplicationPause(bool shouldBePaused) {
      // When pausing, try to close any challenge that's open and reset robot state to Idle
      if (!_IsPaused && shouldBePaused) {
        DAS.Debug("PauseManager.HandleApplicationPause", "Application being paused");
        _IsPaused = true;

        HubWorldBase hub = HubWorldBase.Instance;
        if (null != hub) {
          hub.CloseChallengeImmediately();
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
      Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
      if (null != robot) {
        robot.DisableAllReactionsWithLock(ReactionaryBehaviorEnableGroups.kPauseManagerId);
      }
      StartIdleTimeout(Settings.PlayerSleepCozmo_TimeTilSleep_sec, Settings.PlayerSleepCozmo_TimeTilDisconnect_sec);
      OpenGoToSleepDialogAndFreezeUI();
    }

    private void HandleFinishedWakeup(bool success) {
      HubWorldBase hub = HubWorldBase.Instance;
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;

      if (null != robot && null != hub) {
        hub.StartFreeplay(robot);
        robot.EnableCubeSleep(false);
      }
      else {
        // If this is fired because we are returning from backgrounding the app
        // but Cozmo has disconnected, CloseAllDialogs and handle it like a disconnect
        PauseManager.Instance.PauseManagerReset();
        if (IntroManager.Instance != null) {
          IntroManager.Instance.ForceBoot();
        }
        else if (NeedsConnectionManager.Instance != null) {
          NeedsConnectionManager.Instance.ForceBoot();
        }
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
        robot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kPauseManagerId);
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
        AlertModalData confirmSleepCozmoAlert = null;
        if (handleOnChargerSleepCancel) {
          confirmSleepCozmoAlert = new AlertModalData("sleep_cozmo_on_charger_alert",
                                                      LocalizationKeys.kSettingsSleepCozmoPanelConfirmationModalTitle,
                                                      LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalDescription,
                                                      new AlertModalButtonData("confirm_sleep_button",
                                                                               LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalButtonConfirm,
                                                                               HandleConfirmSleepCozmoOnChargerButtonTapped,
                                                                               Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.AudioMetaData.GameEvent.Ui.Cozmo_Disconnect)),
                                                      new AlertModalButtonData("cancel_sleep_button", LocalizationKeys.kButtonCancel, HandleOnChargerSleepCancel));

        }
        else {
          confirmSleepCozmoAlert = new AlertModalData("sleep_cozmo_from_settings_alert",
                                                      LocalizationKeys.kSettingsSleepCozmoPanelConfirmationModalTitle,
                                                      LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalDescription,
                                                      new AlertModalButtonData("confirm_sleep_button",
                                                                               LocalizationKeys.kSettingsSleepCozmoPanelConfirmModalButtonConfirm,
                                                                               HandleConfirmSleepCozmoButtonTapped,
                                                                               Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.AudioMetaData.GameEvent.Ui.Cozmo_Disconnect)),
                                                      new AlertModalButtonData("cancel_sleep_button", LocalizationKeys.kButtonCancel));
        }

        var confirmSleepCozmoPriority = new ModalPriorityData(ModalPriorityLayer.High, 1,
                                                              LowPriorityModalAction.Queue,
                                                              HighPriorityModalAction.ForceCloseOthersAndOpen);

        Action<AlertModal> confirmSleepCreated = (alertModal) => {
          _SleepCozmoConfirmDialog = alertModal;
          alertModal.OpenAudioEvent = Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.AudioMetaData.GameEvent.Ui.Attention_Device);
        };

        UIManager.OpenAlert(confirmSleepCozmoAlert, confirmSleepCozmoPriority, confirmSleepCreated,
                            overrideBackgroundDim: null,
                            overrideCloseOnTouchOutside: false);
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
        var goToSleepAlertData = new AlertModalData("cozmo_going_to_sleep_alert",
                                                    LocalizationKeys.kConnectivityCozmoSleepTitle,
                                                    LocalizationKeys.kConnectivityCozmoSleepDesc);

        var goToSleepPriority = new ModalPriorityData(ModalPriorityLayer.VeryHigh, 0,
                                                      LowPriorityModalAction.Queue,
                                                      HighPriorityModalAction.ForceCloseOthersAndOpen);

        Action<AlertModal> goToSleepAlertCreated = (alertModal) => {
          _GoToSleepDialog = alertModal;
          // Set Music State
          Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Sleep);
        };

        UIManager.OpenAlert(goToSleepAlertData, goToSleepPriority, goToSleepAlertCreated,
                            overrideCloseOnTouchOutside: false);
      }
    }

    private void OpenLowBatteryAlert() {
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        return;
      }
      if (!IsLowBatteryDialogOpen) {
        var lowBatteryAlertData = new AlertModalData("low_battery_alert",
                                                     LocalizationKeys.kConnectivityCozmoLowBatteryTitle,
                                                     LocalizationKeys.kConnectivityCozmoLowBatteryDesc,
                                                     showCloseButton: true);

        Action<BaseModal> lowBatteryAlertCreated = (newModal) => {
          AlertModal alertModal = (AlertModal)newModal;
          alertModal.InitializeAlertData(lowBatteryAlertData);
          _LowBatteryAlertInstance = alertModal;
        };

        var lowBatteryPriorityData = new ModalPriorityData(ModalPriorityLayer.VeryLow, 0,
                                                           LowPriorityModalAction.Queue,
                                                           HighPriorityModalAction.Queue);
        DAS.Debug("PauseManager.OpenLowBatteryAlert", "Opening Low Battery Alert");
        UIManager.OpenModal(_LowBatteryAlertPrefab, lowBatteryPriorityData, lowBatteryAlertCreated);
        // this needs to be set right away otherwise the low battery listener will call OpenLowBatteryAlert a ton of times.
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
        _SleepCozmoConfirmDialog.CloseDialog();
        _SleepCozmoConfirmDialog = null;
      }
    }

    private void CloseGoToSleepDialog() {
      if (null != _GoToSleepDialog) {
        _GoToSleepDialog.CloseDialog();
        _GoToSleepDialog = null;
      }
    }

    private void CloseLowBatteryDialog() {
      if (null != _LowBatteryAlertInstance) {
        _LowBatteryAlertInstance.CloseDialog();
        _LowBatteryAlertInstance = null;
      }
    }
  }
}
