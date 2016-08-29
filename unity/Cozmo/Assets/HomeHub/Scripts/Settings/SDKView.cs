using Cozmo.UI;
using DataPersistence;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Settings {
  public class SDKView : BaseView {

    [SerializeField]
    private CozmoButton _DisableSDKButton;

    [SerializeField]
    private Text _SDKMessageOutput;

    [SerializeField]
    private Text _TimeSinceLastMessageLabel;

    [SerializeField]
    private Text _ConnectionStatusLabel;

    private bool _BackgroundTimeoutSent = false;

    // Use this for initialization
    private void Start() {
      // Send EnterSDKMode to engine as we enter this view
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.EnterSDKMode();
      }

#if SDK_ONLY
      // Hide DisableSDKButton if this is an SDK_ONLY build
      _DisableSDKButton.gameObject.SetActive(false);
#endif

      _SDKMessageOutput.text = "...";
      _TimeSinceLastMessageLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelNoMessagesText);

      _DisableSDKButton.Initialize(HandleDisableSDKButtonTapped, "disable_sdk_button", "sdk_view");
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(OnRobotDisconnect);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.SdkStatus>(HandleSDKMessageReceived);
    }



    private void HandleDisableSDKButtonTapped() {
      DAS.Info("SDKView.HandleDisableSDKButtonTapped", "Disable SDK Button tapped!");
      DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled = false;
      DataPersistenceManager.Instance.Save();
      CloseView();
    }

    private void HandleSDKMessageReceived(Anki.Cozmo.ExternalInterface.SdkStatus message) {
      if (message.timeSinceLastSdkMessage_s >= 0.0f) {
        _SDKMessageOutput.text = message.sdkStatus;
        _TimeSinceLastMessageLabel.text = Localization.GetWithArgs(LocalizationKeys.kSettingsSdkPanelLastMessageTimeText, message.timeSinceLastSdkMessage_s);
      }
      else {
        _SDKMessageOutput.text = "...";
        _TimeSinceLastMessageLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelNoMessagesText);
      }
      if (message.isConnected) {
        _ConnectionStatusLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelSdkConnectedText);
      }
      else {
        _ConnectionStatusLabel.text = Localization.Get(LocalizationKeys.kSettingsSdkPanelSdkNotConnectedText);
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
      CloseViewImmediately();
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