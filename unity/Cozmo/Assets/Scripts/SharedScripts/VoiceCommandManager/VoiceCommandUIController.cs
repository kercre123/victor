using Cozmo.Needs.UI;
using Cozmo.UI;
using UnityEngine;

namespace Anki.Cozmo.VoiceCommand {
  public class VoiceCommandUIController : MonoBehaviour {
    [SerializeField]
    private NeedsHubView _NeedsHubView;

    [SerializeField]
    private CozmoButton _VoiceSettingsButton;

    [SerializeField]
    private CozmoButton _VoiceSettingsOffButton;

    [SerializeField]
    private VoiceSettingsModal _VoiceSettingsModalPrefab;
    private VoiceSettingsModal _VoiceSettingsModalInstance;

    [SerializeField]
    private VoicePrePromptModal _VoicePrePromptModalPrefab;
    private VoicePrePromptModal _VoicePrePromptModalInstance;

    private void Start() {
      _VoiceSettingsButton.Initialize(HandleVoiceSettingsButton, "voice_settings_button", _NeedsHubView.DASEventDialogName);
      _VoiceSettingsOffButton.Initialize(HandleVoiceSettingsButton, "voice_settings_button_off", _NeedsHubView.DASEventDialogName);
      _VoiceSettingsButton.gameObject.SetActive(false);
      _VoiceSettingsOffButton.gameObject.SetActive(true);

      VoiceCommandManager.Instance.StateDataCallback += HandleVoiceCommandStateDataUpdate;
      VoiceCommandManager.RequestCurrentStateData();
    }

    private void OnDestroy() {
      VoiceCommandManager.Instance.StateDataCallback -= HandleVoiceCommandStateDataUpdate;
      if (_VoicePrePromptModalInstance != null) {
        _VoicePrePromptModalInstance.OnOSPromptClosed -= HandleOSPromptClosed;
      }
    }

    private void HandleVoiceSettingsButton() {
      if (VoiceCommandManager.IsMicrophoneAuthorized) {
        UIManager.OpenModal(_VoiceSettingsModalPrefab, new ModalPriorityData(), HandleVoiceSettingsModalCreated);
      }
      else {
        UIManager.OpenModal(_VoicePrePromptModalPrefab, new ModalPriorityData(), HandlePrePromptModalCreated);
      }
    }

    private void HandleVoiceCommandStateDataUpdate(StateData stateData) {
      bool isEnabled = VoiceCommandManager.IsVoiceCommandsEnabled(stateData);
      _VoiceSettingsButton.gameObject.SetActive(isEnabled);
      _VoiceSettingsOffButton.gameObject.SetActive(!isEnabled);
    }

    private void HandleVoiceSettingsModalCreated(BaseModal newModal) {
      _VoiceSettingsModalInstance = (VoiceSettingsModal)newModal;
      _VoiceSettingsModalInstance.InitializeVoiceSettingsModal();
    }

    private void HandlePrePromptModalCreated(BaseModal newModal) {
      _VoicePrePromptModalInstance = (VoicePrePromptModal)newModal;
      _VoicePrePromptModalInstance.OnOSPromptClosed += HandleOSPromptClosed;
    }

    private void HandleOSPromptClosed() {
      _VoicePrePromptModalInstance.CloseDialog();
      UIManager.OpenModal(_VoiceSettingsModalPrefab, new ModalPriorityData(), HandleVoiceSettingsModalCreated);
    }
  }
}
