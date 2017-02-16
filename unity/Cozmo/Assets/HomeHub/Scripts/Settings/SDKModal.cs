using Cozmo.UI;
using DataPersistence;
using System;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SDKModal : BaseModal {

    [SerializeField]
    private CozmoButton _DisableSDKButton;

    [SerializeField]
    private Text _SDKMessageOutput;

    [SerializeField]
    private Text _TimeSinceLastMessageLabel;

    [SerializeField]
    private Text _ConnectionStatusLabel;

    // Some SDK users are reporting burn in on their phones.
    // Allow this setting for "screensaver mode"
    // We can't use Unity's Screen.sleepTimeout because it makes the app disconnect
    [SerializeField]
    private CozmoButton _HideScreenButton;

    [SerializeField]
    private CozmoButton _ShowScreenButton;

    [SerializeField]
    private GameObject _ParentScreenElements;

    private bool _BackgroundTimeoutSent = false;

    private void Awake() {
      _DisableSDKButton.Initialize(HandleDisableSDKButtonTapped, "disable_sdk_button", "sdk_view");

      _HideScreenButton.Initialize(HandleHideScreenButtonTapped, "hide_screen_sdk_button", "sdk_view");

      _ShowScreenButton.Initialize(HandleShowScreenButtonTapped, "show_screen_sdk_button", "sdk_view");

      ShowElements(true);
    }

    // Use this for initialization
    private void Start() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      // Send EnterSDKMode to engine as we enter this view
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.EnterSDKMode();
      }

#if SDK_ONLY
      // Hide DisableSDKButton if this is an SDK_ONLY build
      _DisableSDKButton.gameObject.SetActive(false);
#endif

      _SDKMessageOutput.text = "";
      _TimeSinceLastMessageLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelNoMessagesText);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.SdkStatus>(HandleSDKMessageReceived);
    }



    private void HandleDisableSDKButtonTapped() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
      DAS.Info("SDKView.HandleDisableSDKButtonTapped", "Disable SDK Button tapped!");
      DataPersistenceManager.Instance.IsSDKEnabled = false;
      DataPersistenceManager.Instance.Save();
      CloseDialog();
    }

    private string SecondsToDateTimeString(float inSeconds) {
      TimeSpan timespan = TimeSpan.FromSeconds(inSeconds);
      // Displays as [-][d:]h:mm:ss[.FFFFFFF], but culturally sensitive
      return string.Format(Localization.GetCultureInfo(), "0:g", timespan);
    }

    private void HandleShowScreenButtonTapped() {
      ShowElements(true);
    }
    private void HandleHideScreenButtonTapped() {
      ShowElements(false);
    }
    // Screen saver burn in mode, tap to show
    private void ShowElements(bool show) {
      _ShowScreenButton.gameObject.SetActive(!show);
      _ParentScreenElements.SetActive(show);
    }

    private void HandleSDKMessageReceived(Anki.Cozmo.ExternalInterface.SdkStatus message) {

      _SDKMessageOutput.text = Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelModeDurationText,
                                                           SecondsToDateTimeString(message.timeInSdkMode_s)) + "\n";
      _SDKMessageOutput.text += Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelProgramsRunText,
                                                           message.numTimesConnected) + "\n\n";

      if (message.timeSinceLastSdkMessage_s >= 0.0f) {
        _TimeSinceLastMessageLabel.text = Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelLastMessageTimeText, message.timeSinceLastSdkMessage_s);
      }
      else {
        _TimeSinceLastMessageLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelNoMessagesText);
      }

      const float kConnectionTimeoutThreshold_s = 5.0f;

      if (message.connectionStatus.isConnected) {
        _ConnectionStatusLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelSdkConnectedText);

        _SDKMessageOutput.text += Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelConnectionDurationText,
                                                           SecondsToDateTimeString(message.connectionStatus.timeInCurrentConnection_s)) + "\n";
        _SDKMessageOutput.text += Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelConnectionCommandCountText,
                                                           Localization.GetNumber((int)message.connectionStatus.numCommands)) + "\n";

        if (message.timeSinceLastSdkCommand_s >= 0.0f) {
          if (message.timeSinceLastSdkCommand_s >= kConnectionTimeoutThreshold_s) {
            if (message.timeSinceLastSdkMessage_s >= kConnectionTimeoutThreshold_s) {
              // Connection may have timed out (we don't close it, just message this to user)
              _SDKMessageOutput.text += Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelLastCommandTimePausedText,
                                                                       message.timeSinceLastSdkCommand_s) + "\n";
            }
            else {
              // Connection is still active, just no commands - message as idle
              _SDKMessageOutput.text += Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelLastCommandTimeIdleText,
                                                                       message.timeSinceLastSdkCommand_s) + "\n";
            }
          }
          else {
            _SDKMessageOutput.text += Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelLastCommandTimeText,
                                                                     message.timeSinceLastSdkCommand_s) + "\n";
          }
        }
      }
      else {
        _ConnectionStatusLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelSdkNotConnectedText);
        if (message.connectionStatus.isWrongSdkVersion) {
          _SDKMessageOutput.text += Localization.Get(LocalizationKeys.kSettingsSdkPanelWrongVersionText) + "\n";
        }
      }

      _SDKMessageOutput.text += "\n";

      for (int i = 0; i < message.sdkStatus.Length; ++i) {
        var sdkStatus = message.sdkStatus[i];
        Anki.Cozmo.SdkStatusType statusType = (Anki.Cozmo.SdkStatusType)i;
        _SDKMessageOutput.text += statusType.ToString() + ": " + sdkStatus + "\n";
      }

    }

    // If we background the app while in SDK View, immediately time out
    private void OnApplicationPause(bool bPause) {
      if (!_BackgroundTimeoutSent) {
        Robot robot = (Robot)RobotEngineManager.Instance.CurrentRobot;
        if (null != robot) {
          robot.ResetRobotState(null);
          robot.EnableCubeSleep(true);
          // Immediately send Exit SDK Mode here since we want SDK disabled before we unpause the app.
          // The IdleTimeout will handle the rest
          robot.ExitSDKMode();
        }

        RobotEngineManager.Instance.StartIdleTimeout(0.0f, 0.0f);
        _BackgroundTimeoutSent = true;
      }
    }

    private void OnRobotDisconnect(object message) {
      CloseDialogImmediately();
    }

    protected override void CleanUp() {
      base.CleanUp();
      // Send ExitSDKMode to engine as we clean up this view
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.ExitSDKMode();
      }
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.SdkStatus>(HandleSDKMessageReceived);
    }

    // TODO: STRETCH GOAL/DEC FEATURE: Use COZMO's Viz Debug Screen and camera feed here?

  }
}