//#define VOICE_COMMAND_ENABLED

using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;
using Anki.Cozmo.VoiceCommand;

namespace Cozmo {
  public class VoiceCommandPane : MonoBehaviour {

    [SerializeField]
    private Button _ToggleEnabledButton;

    [SerializeField]
    private Text _EnabledStatusText;

    [SerializeField]
    private Text _StatusExplanationText;

    [SerializeField]
    private Toggle _ToggleMicIgnored;

    [SerializeField]
    private Button _HeyCozmoButton;

    [SerializeField]
    private Button _LetsPlayButton;


    // Local variable defaults to false until it gets updated by a message from the engine, which is requested in Start()
    private bool _IsEnabled = false;

    private const string _IsEnabledString = "Enabled";
    private const string _IsDisabledString = "Disabled";
    private const string _DeniedNoRetryExplanation = "Microphone access is denied. Go to Settings to enable microphone access for Cozmo.";
    private const string _DeniedAllowRetryExplanation = "Microphone access denied. Try again and allow access to use Voice Commands. If no prompt appears check Settings.";

    private const string _kIgnoreMicKey = "IgnoreMicInput";
    private const string _kHeyCozmoFuncKey = "HearHeyCozmo";
    private const string _kLetsPlayFuncKey = "HearLetsPlay";

    // Use this for initialization
    void Start() {
      _ToggleEnabledButton.onClick.AddListener(OnToggleButton);
      _ToggleMicIgnored.onValueChanged.AddListener(OnMicIgnoredToggle);
      _HeyCozmoButton.onClick.AddListener(OnHeyCozmoButton);
      _LetsPlayButton.onClick.AddListener(OnLetsPlayButton);

      VoiceCommandManager.Instance.StateDataCallback += UpdateStateData;
      VoiceCommandManager.SendVoiceCommandEvent<RequestStatusUpdate>(Singleton<RequestStatusUpdate>.Instance);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleIgnoreMicInput);
      GetDebugConsoleVarMessage initializedGetMessage = Singleton<GetDebugConsoleVarMessage>.Instance.Initialize(_kIgnoreMicKey);
      RobotEngineManager.Instance.Message.Initialize(initializedGetMessage);
      RobotEngineManager.Instance.SendMessage();
    }

    void OnDestroy() {
      VoiceCommandManager.Instance.StateDataCallback -= UpdateStateData;
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleIgnoreMicInput);

      _LetsPlayButton.onClick.RemoveListener(OnLetsPlayButton);
      _HeyCozmoButton.onClick.RemoveListener(OnHeyCozmoButton);
      _ToggleMicIgnored.onValueChanged.RemoveListener(OnMicIgnoredToggle);
      _ToggleEnabledButton.onClick.RemoveListener(OnToggleButton);
    }

    private void OnToggleButton() {
    #if VOICE_COMMAND_ENABLED
      VoiceCommandManager.SendVoiceCommandEvent<ChangeEnabledStatus>(Singleton<ChangeEnabledStatus>.Instance.Initialize(!_IsEnabled));
    #endif // VOICE_COMMAND_ENABLED
    }

    private void OnMicIgnoredToggle(bool newValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar(_kIgnoreMicKey, newValue.ToString());
    }

    private void OnHeyCozmoButton() {
      RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kHeyCozmoFuncKey, "");
    }

    private void OnLetsPlayButton() {
      RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kLetsPlayFuncKey, "");
    }
      
    private void HandleIgnoreMicInput(Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage message) {
      if (message.varName == _kIgnoreMicKey && message.success) {
        _ToggleMicIgnored.isOn = message.varValue.varBool == 0 ? false : true;
      }
    }

    private void UpdateStateData(StateData stateData) {
      _IsEnabled = stateData.isVCEnabled;
      _EnabledStatusText.text = _IsEnabled ? _IsEnabledString : _IsDisabledString;
      _EnabledStatusText.color = _IsEnabled ? Color.green : Color.red;

      _StatusExplanationText.text = "";
      switch (stateData.capturePermissionState) {
      case AudioCapturePermissionState.DeniedAllowRetry:
        {
          _StatusExplanationText.text = _DeniedAllowRetryExplanation;
          break;
        }
      case AudioCapturePermissionState.DeniedNoRetry:
        {
          _StatusExplanationText.text = _DeniedNoRetryExplanation;
          break;
        }
      default:
        {
          break;
        }
      }
    }
  }
}
