using UnityEngine;
using System.Collections;

public class ConnectionFlowBackgroundModal : Cozmo.UI.BaseModal {

  [SerializeField]
  private Sprite _CompletedStateSprite;

  [SerializeField]
  private Sprite _CompletedStateBackground;

  [SerializeField]
  private Sprite _InProgressBackground;

  [SerializeField]
  private Sprite _FailedSprite;

  [SerializeField]
  private Sprite _FailedBackground;

  [SerializeField]
  private UnityEngine.UI.Image[] _StateImages;

  [SerializeField]
  private UnityEngine.UI.Image[] _StateBackgrounds;

  [SerializeField]
  private GameObject[] _InProgressSpinners;

  private int _CurrentState = 0;

  private void Start() {
    DasTracker.Instance.TrackConnectFlowStarted();
    ResetAllProgress();
  }

  public void ResetAllProgress() {
    for (int i = 0; i < _InProgressSpinners.Length; ++i) {
      _InProgressSpinners[i].SetActive(false);
    }
    for (int i = 0; i < _StateImages.Length; ++i) {
      _StateImages[i].gameObject.SetActive(true);
      _StateImages[i].overrideSprite = null;
    }
    for (int i = 0; i < _StateBackgrounds.Length; ++i) {
      _StateBackgrounds[i].overrideSprite = null;
      _StateBackgrounds[i].gameObject.SetActive(false);
    }
  }

  public void SetStateInProgress(int currentState) {
    if (currentState < 0 || currentState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetState", "Setting current state out of range");
    }

    _CurrentState = currentState;

    Invoke("SetProgressSpinner", 0.33f);

    _StateBackgrounds[currentState].gameObject.SetActive(true);
    _StateBackgrounds[currentState].overrideSprite = _InProgressBackground;
  }

  private void SetProgressSpinner() {
    _StateImages[_CurrentState].gameObject.SetActive(false);
    _InProgressSpinners[_CurrentState].gameObject.SetActive(true);
  }

  public void SetStateFailed(int failedState) {
    if (failedState < 0 || failedState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetFailedState", "Setting current state out of range");
    }
    _StateImages[failedState].gameObject.SetActive(true);
    _StateImages[failedState].overrideSprite = _FailedSprite;
    _StateBackgrounds[failedState].gameObject.SetActive(true);
    _StateBackgrounds[failedState].overrideSprite = _FailedBackground;

    _InProgressSpinners[failedState].gameObject.SetActive(false);
    CancelInvoke();
  }

  public void SetStateComplete(int completedState) {
    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
    if (completedState < 0 || completedState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetStateComplete", "Setting current state out of range");
    }
    _StateImages[completedState].gameObject.SetActive(true);
    _StateImages[completedState].overrideSprite = _CompletedStateSprite;
    _StateBackgrounds[completedState].gameObject.SetActive(true);
    _StateBackgrounds[completedState].overrideSprite = _CompletedStateBackground;

    _InProgressSpinners[completedState].gameObject.SetActive(false);
    CancelInvoke();
  }

  protected override void CleanUp() {
    DasTracker.Instance.TrackConnectFlowEnded();
  }
}
