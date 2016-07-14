using UnityEngine;
using System.Collections;

public class ConnectingToCozmoScreen : MonoBehaviour {
  public System.Action<bool> OnScreenComplete;
  private bool _RobotConnected = false;

  private void Start() {
    Invoke("ShowScreenComplete", ConnectDialog.kConnectionFlowDelay);
  }

  private void ShowScreenComplete() {
    if (_RobotConnected) {
      if (OnScreenComplete != null) {
        OnScreenComplete(true);
      }
    }
    else {
      // not connected yet, check again
      Invoke("ShowScreenComplete", 0.1f);
    }

  }

  public void RobotConnected() {
    _RobotConnected = true;
  }
}
