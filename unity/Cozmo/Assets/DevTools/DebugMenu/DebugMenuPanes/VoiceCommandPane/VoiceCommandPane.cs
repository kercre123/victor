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
    private UnityEngine.UI.Dropdown _VoiceCommandSelection;

    [SerializeField]
    private Button _IssueVoiceCommandButton;


    // Local variable defaults to false until it gets updated by a message from the engine, which is requested in Start()
    private bool _IsEnabled = false;

    private const string _IsEnabledString = "Enabled";
    private const string _IsDisabledString = "Disabled";
    private const string _DeniedNoRetryExplanation = "Microphone access is denied. Go to Settings to enable microphone access for Cozmo.";
    private const string _DeniedAllowRetryExplanation = "Microphone access denied. Try again and allow access to use Voice Commands. If no prompt appears check Settings.";

    private const string _kIgnoreMicKey = "IgnoreMicInput";
    private const string _kIssueCommandFuncKey = "HearVoiceCommand";

    // Use this for initialization
    void Start() {
      _ToggleEnabledButton.onClick.AddListener(OnToggleButton);
      _ToggleMicIgnored.onValueChanged.AddListener(OnMicIgnoredToggle);
      _HeyCozmoButton.onClick.AddListener(OnHeyCozmoButton);
      _IssueVoiceCommandButton.onClick.AddListener(OnHearVoiceCommandButton);

      VoiceCommandManager.Instance.StateDataCallback += UpdateStateData;
      VoiceCommandManager.SendVoiceCommandEvent<RequestStatusUpdate>(Singleton<RequestStatusUpdate>.Instance);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleIgnoreMicInput);
      GetDebugConsoleVarMessage initializedGetMessage = Singleton<GetDebugConsoleVarMessage>.Instance.Initialize(_kIgnoreMicKey);
      RobotEngineManager.Instance.Message.Initialize(initializedGetMessage);
      RobotEngineManager.Instance.SendMessage();
      PopulateOptions();
    }

    void OnDestroy() {
      VoiceCommandManager.Instance.StateDataCallback -= UpdateStateData;
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleIgnoreMicInput);

      _IssueVoiceCommandButton.onClick.RemoveListener(OnHearVoiceCommandButton);
      _HeyCozmoButton.onClick.RemoveListener(OnHeyCozmoButton);
      _ToggleMicIgnored.onValueChanged.RemoveListener(OnMicIgnoredToggle);
      _ToggleEnabledButton.onClick.RemoveListener(OnToggleButton);
    }

    private void PopulateOptions() {
      List<UnityEngine.UI.Dropdown.OptionData> options = new List<UnityEngine.UI.Dropdown.OptionData>();
      for (int i = 0; i < (System.Enum.GetValues(typeof(VoiceCommandType)).Length - 1); ++i) {
        UnityEngine.UI.Dropdown.OptionData option = new UnityEngine.UI.Dropdown.OptionData();
        option.text = System.Enum.GetValues(typeof(VoiceCommandType)).GetValue(i).ToString();
        options.Add(option);
      }
      _VoiceCommandSelection.AddOptions(options);
    }

    private void OnToggleButton() {
      VoiceCommandManager.SendVoiceCommandEvent<ChangeEnabledStatus>(Singleton<ChangeEnabledStatus>.Instance.Initialize(!_IsEnabled));
    }

    private void OnMicIgnoredToggle(bool newValue) {
      RobotEngineManager.Instance.SetDebugConsoleVar(_kIgnoreMicKey, newValue.ToString());
    }

    private void OnHearVoiceCommandButton() {
      IssueVoiceCommand(GetSelectedVoiceCommand ());
    }

    private void OnHeyCozmoButton() {
      IssueVoiceCommand(VoiceCommandType.HeyCozmo);
    }

    private void IssueVoiceCommand(VoiceCommandType commandType) {
      RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kIssueCommandFuncKey, ((int)commandType).ToString());
    }

    private VoiceCommandType GetSelectedVoiceCommand() {
      return (VoiceCommandType)System.Enum.Parse(typeof(VoiceCommandType), _VoiceCommandSelection.options[_VoiceCommandSelection.value].text);
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
