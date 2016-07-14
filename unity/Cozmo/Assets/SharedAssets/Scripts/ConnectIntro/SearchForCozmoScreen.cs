using UnityEngine;
using System.Collections;

public class SearchForCozmoScreen : MonoBehaviour {

  public System.Action<bool> OnScreenComplete;
  private PingStatus _PingStatus;

  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Start() {
    Invoke("ShowScreenComplete", ConnectionFlow.kConnectionFlowDelay);
  }

  private void ShowScreenComplete() {
    if (OnScreenComplete != null) {
      OnScreenComplete(_PingStatus.GetPingStatus());
    }
  }
}
