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

    [SerializeField]
    private GameObject _VCDisabledArrow;

    [SerializeField]
    private VoiceLearnCommandsModal _VCLearnCommandsModalPrefab;

    [SerializeField]
    private CozmoButton _LearnCommandsButton;

#if UNITY_ANDROID && !UNITY_EDITOR
  private AndroidJavaObject _PermissionUtil;
#endif

    public void InitializeVoiceSettingsModal() {
#if UNITY_ANDROID && !UNITY_EDITOR
    _PermissionUtil = new AndroidJavaClass("com.anki.util.PermissionUtil");
#endif

      _SwitchButton.Initialize(HandleSwitchButtonClicked, "enable_voice_commands_button", DASEventDialogName);
      _AppSettingsButton.Initialize(HandleAppSettingsClicked, "open_app_settings_button", DASEventDialogName);
      _LearnCommandsButton.Initialize(HandleLearnCommandsButtonClicked, "open_learn_commands_button", DASEventDialogName);

      UpdateStatus();
      _VCDisabledArrow.SetActive(false);

      VoiceCommandManager.Instance.StateDataCallback += OnMicrophoneAuthorizationStatusUpdate;
      VoiceCommandManager.RequestCurrentStateData();
    }

    protected override void CleanUp() {
      base.CleanUp();
      VoiceCommandManager.Instance.StateDataCallback -= OnMicrophoneAuthorizationStatusUpdate;
    }

    private void OnMicrophoneAuthorizationStatusUpdate(StateData stateData) {
      UpdateStatus();
    }

    private void HandleSwitchButtonClicked() {
      // Emulate an "App Settings" press if mic permission not available
      if (!VoiceCommandManager.IsMicrophoneAuthorized) {
        HandleAppSettingsClicked();
      }
      else {
        VoiceCommandManager.SetVoiceCommandEnabledInApp(!VoiceCommandManager.IsVoiceCommandsEnabledInApp);
        UpdateStatus();
      }
    }

    private void UpdateStatus() {
      if (!VoiceCommandManager.IsMicrophoneAuthorized) {
        _VCEnabledDescription.SetActive(false);
        _VCDisabledDescription.SetActive(true);

        _VCEnabledSwitch.SetActive(false);
        _VCDisabledSwitch.SetActive(true);

        _AuthorizationNeededShelf.SetActive(true);
        _LearnCommandsShelf.SetActive(false);
        _VCDisabledShelf.SetActive(false);
        _VCDisabledArrow.SetActive(false);
      }
      else {
        if (VoiceCommandManager.IsVoiceCommandsEnabledInApp) {
          _VCEnabledDescription.SetActive(true);
          _VCDisabledDescription.SetActive(false);

          _VCEnabledSwitch.SetActive(true);
          _VCDisabledSwitch.SetActive(false);

          _AuthorizationNeededShelf.SetActive(false);
          _LearnCommandsShelf.SetActive(true);
          _VCDisabledShelf.SetActive(false);
          _VCDisabledArrow.SetActive(false);
        }
        else {
          _VCEnabledDescription.SetActive(false);
          _VCDisabledDescription.SetActive(true);

          _VCEnabledSwitch.SetActive(false);
          _VCDisabledSwitch.SetActive(true);

          _AuthorizationNeededShelf.SetActive(false);
          _LearnCommandsShelf.SetActive(false);
          _VCDisabledShelf.SetActive(true);
          _VCDisabledArrow.SetActive(true);
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

    private void HandleLearnCommandsButtonClicked() {
      UIManager.OpenModal(_VCLearnCommandsModalPrefab, ModalPriorityData.CreateSlightlyHigherData(PriorityData), null);
    }
  }
}