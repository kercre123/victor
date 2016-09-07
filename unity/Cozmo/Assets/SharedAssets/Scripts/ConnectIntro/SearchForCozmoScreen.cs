using UnityEngine;
using System.Collections;

public class SearchForCozmoScreen : MonoBehaviour {

  public System.Action<bool> OnScreenComplete;
  private PingStatus _PingStatus;

  [SerializeField]
  private int _AttemptsBeforeShowingFailScreen = 2;

  private int _PingCheckAttempts = 0;

  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Start() {
    Invoke("CheckForConnection", ConnectionFlow.ConnectionFlowDelay);
  }

  private void CheckForConnection() {
    _PingCheckAttempts++;
    if (_PingStatus.GetPingStatus() || _PingCheckAttempts >= _AttemptsBeforeShowingFailScreen) {
      ShowScreenComplete();
    }
    else {
      Invoke("CheckForConnection", ConnectionFlow.ConnectionFlowDelay);
    }
  }

  private void ShowScreenComplete() {
    if (OnScreenComplete != null) {
      OnScreenComplete(_PingStatus.GetPingStatus());
    }
  }
}
