using Cozmo.UI;
using DataPersistence;
using UnityEngine;

namespace Anki.Cozmo.VoiceCommand {
  public class VoiceSettingsModal : BaseModal {

    [SerializeField]
    private CozmoText _StatusLabel;
    [SerializeField]
    private CozmoButton _SwitchButton;
    [SerializeField]
    private CozmoButton _AppSettingsButton;

    [SerializeField]
    private GameObject _VCEnabledDescription;
    [SerializeField]
    private GameObject _VCDisabledDescription;

    [SerializeField]
    private GameObject _VCEnabledSwitch;
    [SerializeField]
    private GameObject _VCDisabledSwitch;

    [SerializeField]
    private GameObject _AuthorizationNeededShelf;
    [SerializeField]
    private GameObject _LearnCommandsShelf;
    [SerializeField]
    private GameObject _VCDisabledShelf;

    private AudioCapturePermissionState _CurrentAudioCapturePermissionState;

#if UNITY_ANDROID && !UNITY_EDITOR
  private AndroidJavaObject _PermissionUtil;
#endif

    public void InitializeVoiceSettingsModal() {
#if UNITY_ANDROID && !UNITY_EDITOR
    _PermissionUtil = new AndroidJavaClass("com.anki.util.PermissionUtil");
#endif

      _SwitchButton.Initialize(HandleSwitchButtonClicked, "enable_voice_commands_button", DASEventDialogName);
      _AppSettingsButton.Initialize(HandleAppSettingsClicked, "app_settings_button", DASEventDialogName);

      _CurrentAudioCapturePermissionState = AudioCapturePermissionState.Unknown;
      VoiceCommandManager.Instance.StateDataCallback += OnMicrophoneAuthorizationStatusUpdate;
      VoiceCommandManager.SendVoiceCommandEvent<RequestStatusUpdate>(Singleton<RequestStatusUpdate>.Instance);

      UpdateStatus();
    }

    protected override void CleanUp() {
      base.CleanUp();
      VoiceCommandManager.Instance.StateDataCallback -= OnMicrophoneAuthorizationStatusUpdate;
    }

    private void OnMicrophoneAuthorizationStatusUpdate(StateData stateData) {
      _CurrentAudioCapturePermissionState = stateData.capturePermissionState;
      UpdateStatus();
    }

    private void HandleSwitchButtonClicked() {
      VoiceCommandManager.SetVoiceCommandEnabled(!IsMicrophoneEnabled);
      UpdateStatus();
    }

    private void UpdateStatus() {
      if (IsMicrophoneAuthorizationDenied) {
        _VCEnabledDescription.SetActive(false);
        _VCDisabledDescription.SetActive(true);

        _VCEnabledSwitch.SetActive(false);
        _VCDisabledSwitch.SetActive(true);

        _AuthorizationNeededShelf.SetActive(true);
        _LearnCommandsShelf.SetActive(false);
        _VCDisabledShelf.SetActive(false);
      }
      else {
        if (IsMicrophoneEnabled) {
          _VCEnabledDescription.SetActive(true);
          _VCDisabledDescription.SetActive(false);

          _VCEnabledSwitch.SetActive(true);
          _VCDisabledSwitch.SetActive(false);

          _AuthorizationNeededShelf.SetActive(false);
          _LearnCommandsShelf.SetActive(true);
          _VCDisabledShelf.SetActive(false);
        }
        else {
          _VCEnabledDescription.SetActive(false);
          _VCDisabledDescription.SetActive(true);

          _VCEnabledSwitch.SetActive(false);
          _VCDisabledSwitch.SetActive(true);

          _AuthorizationNeededShelf.SetActive(false);
          _LearnCommandsShelf.SetActive(false);
          _VCDisabledShelf.SetActive(true);
        }
      }
    }

    private void HandleAppSettingsClicked() {
#if !UNITY_EDITOR
#if UNITY_IOS
      IOS_Settings.OpenAppSettings();
#elif UNITY_ANDROID
      _PermissionUtil.CallStatic("openAppSettings");
#else
      DAS.Error("VoiceSettingsModal.HandleAppSettingsClicked", "Platform not supported");
#endif
#endif
    }

    private bool IsMicrophoneAuthorized {
      get {
        return _CurrentAudioCapturePermissionState == AudioCapturePermissionState.Granted;
      }
    }

    private bool IsMicrophoneAuthorizationDenied {
      get {
        if (_CurrentAudioCapturePermissionState == AudioCapturePermissionState.DeniedNoRetry ||
           _CurrentAudioCapturePermissionState == AudioCapturePermissionState.DeniedAllowRetry) {
          return true;
        }
        else {
          return false;
        }
      }
    }

    private bool IsMicrophoneEnabled {
      get {
        return DataPersistenceManager.Instance.Data.DefaultProfile.VoiceCommandEnabledState == VoiceCommandEnabledState.Enabled;
      }
    }
  }
}