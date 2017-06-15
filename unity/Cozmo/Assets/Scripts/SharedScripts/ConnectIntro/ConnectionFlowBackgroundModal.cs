using UnityEngine;
using System.Collections;

public class ConnectionFlowBackgroundModal : Cozmo.UI.BaseModal {
  [SerializeField]
  private GameObject[] _StateImages;
  [SerializeField]
  private GameObject[] _InProgressIndicators;
  [SerializeField]
  private GameObject[] _SuccessIndicators;
  [SerializeField]
  private GameObject[] _FailureIndicators;

  [SerializeField]
  private GameObject[] _InProgressSpinners;

  private int _CurrentState = 0;

  private void Awake() {
    DasTracker.Instance.TrackConnectFlowStarted();
    ResetAllProgress();
  }

  public void ResetAllProgress() {
    for (int i = 0; i < _InProgressSpinners.Length; ++i) {
      _InProgressSpinners[i].SetActive(false);
    }
    for (int i = 0; i < _InProgressIndicators.Length; ++i) {
      _InProgressIndicators[i].SetActive(true);
    }
    for (int i = 0; i < _StateImages.Length; ++i) {
      _StateImages[i].SetActive(true);
    }
    for (int i = 0; i < _SuccessIndicators.Length; ++i) {
      _SuccessIndicators[i].SetActive(false);
    }
    for (int i = 0; i < _FailureIndicators.Length; ++i) {
      _FailureIndicators[i].SetActive(false);
    }
  }

  public void SetStateInProgress(int currentState) {
    if (currentState < 0 || currentState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetState", "Setting current state out of range");
    }

    _CurrentState = currentState;

    Invoke("SetProgressSpinner", 0.33f);

    _StateImages[currentState].SetActive(false);
    _InProgressIndicators[currentState].SetActive(true);
  }

  private void SetProgressSpinner() {
    _StateImages[_CurrentState].gameObject.SetActive(false);
    _InProgressSpinners[_CurrentState].gameObject.SetActive(true);
  }

  public void SetStateFailed(int failedState) {
    if (failedState < 0 || failedState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetFailedState", "Setting current state out of range");
    }
    _StateImages[failedState].SetActive(false);
    _InProgressIndicators[failedState].SetActive(false);
    _FailureIndicators[failedState].SetActive(true);
    _InProgressSpinners[failedState].gameObject.SetActive(false);

    CancelInvoke();
  }

  public void SetStateComplete(int completedState) {
    Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.AudioMetaData.GameEvent.Ui.Cozmo_Connect);
    if (completedState < 0 || completedState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetStateComplete", "Setting current state out of range");
    }
    _StateImages[completedState].SetActive(false);
    _InProgressIndicators[completedState].SetActive(false);
    _SuccessIndicators[completedState].SetActive(true);
    _InProgressSpinners[completedState].gameObject.SetActive(false);

    CancelInvoke();
  }

  protected override void CleanUp() {
    DasTracker.Instance.TrackConnectFlowEnded();
  }
}
