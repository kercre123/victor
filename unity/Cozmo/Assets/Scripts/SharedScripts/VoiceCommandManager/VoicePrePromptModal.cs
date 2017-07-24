using Cozmo.UI;
using UnityEngine;

namespace Anki.Cozmo.VoiceCommand {
  public class VoicePrePromptModal : BaseModal {
    public delegate void PromptAcceptedHandler();
    public event PromptAcceptedHandler OnOSPromptClosed;

    [SerializeField]
    private CozmoButton _MaybeLaterButton;

    [SerializeField]
    private CozmoButton _OkayButton;

    private void Start() {
      _MaybeLaterButton.Initialize(HandleMaybeLaterButtonClicked, "vc_maybe_later_button", DASEventDialogName);
      _OkayButton.Initialize(HandleOkayButtonClicked, "vc_okay_button", DASEventDialogName);
    }

    private void OnDestroy() {
      VoiceCommandManager.Instance.StateDataCallback -= OnMicrophoneAuthorizationStatusUpdate;
    }

    private void HandleMaybeLaterButtonClicked() {
      CloseDialog();
    }

    private void HandleOkayButtonClicked() {
      VoiceCommandManager.Instance.StateDataCallback += OnMicrophoneAuthorizationStatusUpdate;
      // Will show an in-app prompt if possible; sends a StateData when the prompt is closed with updated info.
      VoiceCommandManager.SetVoiceCommandEnabledInApp(true);
    }

    private void OnMicrophoneAuthorizationStatusUpdate(StateData stateData) {
      VoiceCommandManager.Instance.StateDataCallback -= OnMicrophoneAuthorizationStatusUpdate;
      if (OnOSPromptClosed != null) {
        OnOSPromptClosed();
      }
    }
  }
}