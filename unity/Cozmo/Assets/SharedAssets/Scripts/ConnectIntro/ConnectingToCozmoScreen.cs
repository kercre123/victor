using UnityEngine;
using System.Collections;

public class ConnectingToCozmoScreen : MonoBehaviour {
  public System.Action ConnectionScreenComplete;

  private bool _MinimumDelayMet = false;
  private bool _ConnectionComplete = false;

  private void Start() {
    Invoke("MinimumDelayMet", ConnectionFlow.kConnectionFlowDelay);
  }

  private void MinimumDelayMet() {
    _MinimumDelayMet = true;
  }

  private void Update() {
    if (_MinimumDelayMet && _ConnectionComplete) {
      if (ConnectionScreenComplete != null) {
        ConnectionScreenComplete();
      }
    }
  }

  public void ConnectionComplete() {
    _ConnectionComplete = true;
  }

}
